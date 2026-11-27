#include "track.h"

#include <fstream>
#include <sstream>

#include <glm/geometric.hpp>

#include <tuple>

TrackNode TrackSegment::evaluateBezierPoint(const BezierKnot& a, const BezierKnot& b, float t) {
	float one_minus_t = 1.0f - t;
	float one_minus_t_squared = one_minus_t * one_minus_t;
	float one_minus_t_cubed = one_minus_t_squared * one_minus_t;

	float t_squared = t * t;
	float t_cubed = t_squared * t;

	struct TrackNode node;

	/* Calculate position using cubic Bezier formula */
	node.position = one_minus_t_cubed * a.point +
		3 * one_minus_t_squared * t * a.rightHandle +
		3 * one_minus_t * t_squared * b.leftHandle +
		t_cubed * b.point;

	/* Calculate forward vector as the tangent (derivative) of the cubic Bezier formula */
	glm::vec3 derivative = 3 * one_minus_t_squared * (a.rightHandle - a.point) +
		6 * one_minus_t * t * (b.leftHandle - a.rightHandle) +
		3 * t_squared * (b.point - b.leftHandle);

	node.forward = (glm::length(derivative) < 1e-6f) ? 
		glm::normalize(b.point - a.point) :
		glm::normalize(derivative);

	/* Calculate tilt as linear interpulation between the two points */
	node.tilt = one_minus_t * a.tilt + t * b.tilt;

	return node;
}

static glm::vec3 rotateRodrigues(glm::vec3 previous, glm::vec3 axis, float angle) {
	float cosine = glm::cos(angle);

	glm::vec3 rotated = previous * cosine +
		glm::cross(axis, previous) * glm::sin(angle) +
		axis * glm::dot(axis, previous) * (1 - cosine);

	return rotated;
}

TrackNode TrackSegment::applyParallelTransport(TrackNode current, TrackNode previous) {
	glm::vec3 rotationAxis = glm::cross(previous.forward, current.forward);
	float length = glm::length(rotationAxis);

	if (length < 1e-6f) {
		// Keep same up and right if the two forward vectors are almost hidentical
		current.baseUp = previous.baseUp;
	} else {
		float angle = glm::atan(length, glm::dot(previous.forward, current.forward));
		current.baseUp = glm::normalize(rotateRodrigues(previous.baseUp, rotationAxis / length, angle));
	}
	
	current.right = glm::normalize(glm::cross(current.forward, current.baseUp));
	return current;
}

TrackNode TrackSegment::applyTilt(TrackNode current) {
	float sineTilt = glm::sin(current.tilt);
	float cosineTilt = glm::cos(current.tilt);

	glm::vec3 tiltedRight = current.right * cosineTilt + current.baseUp * sineTilt;
	glm::vec3 tiltedUp = current.baseUp * cosineTilt - current.right * sineTilt;

	current.right = tiltedRight;
	current.tiltedUp = tiltedUp;

	return current;
}

static std::vector<BezierKnot> readPathKnots(const std::string& path) {
	std::ifstream file(path);
	if (!file) throw std::runtime_error("File " + path + " cannot be opened.");

	std::vector<BezierKnot> knots;
	std::string line;

	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#') continue;

		std::istringstream ss(line);
		BezierKnot k;
		float tiltDegrees;

		ss >> k.point.x >> k.point.y >> k.point.z >>
			k.leftHandle.x >> k.leftHandle.y >> k.leftHandle.z >>
			k.rightHandle.x >> k.rightHandle.y >> k.rightHandle.z >>
			tiltDegrees;

		k.tilt = glm::radians(tiltDegrees);
		knots.push_back(k);
	}
	return knots;
}

TrackSegment::TrackSegment(std::vector<BezierKnot> knots, int samplesPerCurve) : knots(std::move(knots)) {
	if (this->knots.size() < 2) throw std::invalid_argument("Not enough knots to create a TrackSegment");
	if (samplesPerCurve < 1) samplesPerCurve = 1;

	build(samplesPerCurve);
}

TrackSegment TrackSegment::fromFile(const std::string& path, int samplesPerCurve) {
	return TrackSegment(readPathKnots(path), samplesPerCurve);
}

void TrackSegment::build(int samplesPerCurve) {
	nodes.clear();

	TrackNode prev{};
	bool first = true;

	for (int i = 0; i < knots.size() - 1; ++i) {
		bool last = (i == knots.size() - 2);
		for (int j = 0; j < samplesPerCurve + last; ++j) {
			float t = float(j) / samplesPerCurve;

			TrackNode node = evaluateBezierPoint(knots[i], knots[i + 1], t);

			if (first) {
				glm::vec3 worldUp(0, 1, 0);

				// Assume the track node doesn't start vertically
				node.baseUp = glm::normalize(worldUp - node.forward * glm::dot(worldUp, node.forward));

				node.right = glm::cross(node.forward, node.baseUp);
				first = false;
			} else {
				node = applyParallelTransport(node, prev);
			}

			prev = node;
			nodes.push_back(applyTilt(node));
		}
	}
}

TrackMesh::TrackMesh(const std::vector<TrackNode>& nodes, const TrackProfile& profile) {
	if (nodes.size() < 2) return;

	const std::vector<ProfilePoint>& points = profile.getPoints();
	float width = profile.getWidth();
	
	float distance = 0.0f;
	for (int i = 0; i < nodes.size(); i++) {
		if (i > 0) distance += glm::length(nodes[i].position - nodes[i - 1].position);

		TrackNode n = nodes[i];
		for (int j = 0; j < points.size(); ++j) {
			ProfilePoint p = points[j];
			TrackVertex v;

			v.position = n.position + n.right * p.position.x + n.tiltedUp * p.position.y;
			v.normal = n.right * p.normal.x + n.tiltedUp * p.normal.y;
			v.uv = glm::vec2(p.u / width, distance);
			vertices.push_back(v);
		}
	}

	int P = points.size();
	for (int i = 0; i < nodes.size() - 1; ++i) {
		for (int j = 0; j < P - 1; ++j) {
			unsigned int a = i * P + j;
			unsigned int b = i * P + j + 1;
			unsigned int c = (i + 1) * P + j;
			unsigned int d = (i + 1) * P + j + 1;

			indices.insert(indices.end(), { a, b, c, b, d, c });
		}
	}
}

TrackProfile::TrackProfile(std::vector<BezierKnot2D> knots, bool loop, int samplesPerCurve) : knots(std::move(knots)) { 
	this->loop = loop;
	build(samplesPerCurve);
}

TrackProfile TrackProfile::fromFile(const std::string& path, int samplesPerCurve) {
	std::ifstream file(path);
	if (!file) throw std::runtime_error("File " + path + " cannot be opened.");
	
	std::vector<BezierKnot2D> knots;
	std::string line;
	bool loop = false;

	std::getline(file, line);
	std::istringstream ss(line);
	ss >> loop;	

	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#') continue;

		std::istringstream ss(line);
		BezierKnot2D k;

		ss >> k.point.x >> k.point.y >>
			k.leftHandle.x >> k.leftHandle.y >>
			k.rightHandle.x >> k.rightHandle.y;

		knots.push_back(k);
	}

	TrackProfile profile(std::move(knots), loop, samplesPerCurve);
	return profile;
}

ProfilePoint TrackProfile::evaluateBezierPoints(const BezierKnot2D& a, const BezierKnot2D& b, float t) {
	float one_minus_t = 1.0f - t;
	float one_minus_t_squared = one_minus_t * one_minus_t;
	float one_minus_t_cubed = one_minus_t_squared * one_minus_t;

	float t_squared = t * t;
	float t_cubed = t_squared * t;
	
	ProfilePoint point;

	point.position = one_minus_t_cubed * a.point +
		3 * one_minus_t_squared * t * a.rightHandle +
		3 * one_minus_t * t_squared * b.leftHandle +
		t_cubed * b.point;

	glm::vec2 derivative = 3 * one_minus_t_squared * (a.rightHandle - a.point) +
		6 * one_minus_t * t * (b.leftHandle - a.rightHandle) +
		3 * t_squared * (b.point - b.leftHandle);

	derivative = (glm::dot(derivative, derivative) < 1e-6) ? b.point - a.point : glm::normalize(derivative);
	point.normal = glm::vec2(-derivative.y, derivative.x);

	return point;
}

void TrackProfile::build(int samplesPerCurve) {
	points.clear();

	ProfilePoint prev;
	
	// TODO handle looping

	std::vector<BezierKnot2D> knotsCopy = knots;
	if (loop) knotsCopy.insert(knotsCopy.end(), knots[0]);
	
	float distance = 0.0f;
	for (int i = 0; i < knotsCopy.size() - 1; ++i) {
		bool last = (i == knotsCopy.size() - 2);
		for (int j = 0; j < samplesPerCurve + last; ++j) {
			float t = (float)j / samplesPerCurve;

			ProfilePoint point = evaluateBezierPoints(knotsCopy[i], knotsCopy[i + 1], t);
			
			if (j > 0) { distance += glm::length(point.position - prev.position); }
			point.u = distance;

			points.push_back(point);
			prev = point;
		}
	}

	width = distance;
}

ProfileContact TrackProfile::getProfileContact(glm::vec2 point) const {

	ProfileContact contact{};

	float bestDistanceSquared = FLT_MAX;
	unsigned int bestIndex = 0;
	float best_t = 0.0f;

	for (int i = 0; i < points.size() - 1; ++i) {
		const ProfilePoint& current = points[i];
		const ProfilePoint& next = points[i + 1];

		glm::vec2 dir(next.position - current.position);
		float dd = glm::dot(dir, dir);
		float t = (dd > 1e-12) ? 
			glm::clamp(glm::dot(point - current.position, dir) / dd, 0.0f, 1.0f) : 0;
		
		glm::vec2 diff = point - (current.position + t * dir);
		float distanceSquared = glm::dot(diff, diff);

		if (distanceSquared < bestDistanceSquared) {
			bestDistanceSquared = distanceSquared;
			bestIndex = i;
			best_t = t;
		}
	}

	const ProfilePoint& a = points[bestIndex];
	const ProfilePoint& b = points[bestIndex + 1];

	float one_minus_best_t = 1.0f - best_t;

	contact.index = bestIndex;
	contact.position = one_minus_best_t * a.position + best_t * b.position;
	contact.normal = glm::normalize(one_minus_best_t * a.normal + best_t * b.normal);
	contact.positionAlongProfile = one_minus_best_t * a.u + best_t * b.u;
	contact.signedDistance = glm::dot(point - contact.position, contact.normal);

	return contact;
}

Track::Track(const TrackSegment& segment, TrackProfile profile) : nodes(segment.getNodes()), profile(std::move(profile)) {
	if (nodes.size() < 2) throw new std::invalid_argument("Therack needs to have at least two nodes");
}

TrackContact Track::locate(const glm::vec3& position, TrackCursor& cursor) const {
	const unsigned int last = nodes.size() - 2;
	int i = std::min(cursor.node, last);

	while (i < last && glm::dot(position - nodes[i + 1].position, nodes[i + 1].forward) >= 0.0f) ++i;
	while (i > 0 && glm::dot(position - nodes[i].position, nodes[i].forward) < 0.0f) --i;
	cursor.node = i;

	const TrackNode& current = nodes[i];
	const TrackNode& next = nodes[i + 1];

	float currentForwardOffset = glm::dot(position - current.position, current.forward);
	float nextForwardOffset = glm::dot(position - next.position, next.forward);

	float lambda = (currentForwardOffset < 0.0f) ? 0.0f :
		(nextForwardOffset > 1.0f) ? 1.0f :
		currentForwardOffset / (currentForwardOffset - nextForwardOffset);

	float one_minus_lambda = 1.0f - lambda;
	glm::vec3 pos = one_minus_lambda * current.position + lambda * next.position;
	glm::vec3 fwd = glm::normalize(one_minus_lambda * current.forward + lambda * next.position);
	glm::vec3 up = one_minus_lambda * current.tiltedUp + lambda * next.tiltedUp;
	up = glm::normalize(up - fwd * glm::dot(up, fwd)); // Ortonormalizzazione
	glm::vec3 right = glm::cross(fwd, up);

	glm::vec3 diff = position - pos;
	glm::vec2 localPosition(glm::dot(diff, right), glm::dot(diff, up));
	ProfileContact profileContact = profile.getProfileContact(localPosition);

	TrackContact trackContact;
	trackContact.index = i;
	trackContact.lambda = lambda;
	trackContact.positionAlongProfile = profileContact.positionAlongProfile;
	trackContact.position = pos + right * profileContact.position.x + up * profileContact.position.y;
	trackContact.normal = glm::normalize(right * profileContact.normal.x + up * profileContact.normal.y);
	trackContact.signedDistance = profileContact.signedDistance;

	return trackContact;
}

FindVertexResult Track::find_vertex(const glm::vec3& position, TrackCursor& cursor) const {
	TrackContact contact = locate(position, cursor);
	return { 
		contact.index + contact.lambda, 
		contact.positionAlongProfile, 
		contact.signedDistance, 
		contact.position, 
		contact.normal
	};
}

std::tuple<std::vector<float>, std::vector<unsigned int>> Track::mesh() const {
	TrackMesh mesh(nodes, profile);

	std::vector<float> data;

	data.reserve(mesh.getVertices().size() * 11);
	for (const TrackVertex& v : mesh.getVertices()) {
		data.insert(data.end(), {
			v.position.x, v.position.y, v.position.z,
			v.normal.x, v.normal.y, v.normal.z,
			v.uv.x, v.uv.y,
			v.normal.x, v.normal.y, v.normal.z
		});
	}
	return std::make_tuple(std::move(data), mesh.getIndices());
}