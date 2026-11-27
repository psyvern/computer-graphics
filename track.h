#pragma once

#include <glm/glm.hpp>
#include <glm/matrix.hpp>

#include <vector>
#include <string>

struct BezierKnot {
	glm::vec3 point;
	glm::vec3 leftHandle;
	glm::vec3 rightHandle;
	float tilt;
};

struct BezierKnot2D {
	glm::vec2 point;
	glm::vec2 leftHandle;
	glm::vec2 rightHandle;
};

struct TrackNode {
	glm::vec3 position;
	glm::vec3 forward;

	glm::vec3 tiltedUp;
	glm::vec3 baseUp;

	glm::vec3 right;

	float tilt;
};

struct TrackVertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

struct ProfilePoint {
	glm::vec2 position;
	glm::vec2 normal;
	float u;
};

struct ProfileContact {
	unsigned int index;
	float positionAlongProfile;
	float signedDistance;
	glm::vec2 position;
	glm::vec2 normal;
};

struct TrackContact {
	unsigned int index;
	float lambda;
	float positionAlongProfile;
	float signedDistance;
	glm::vec3 position;
	glm::vec3 normal;
};

struct FindVertexResult {
	float t;
	float s;
	float distance;
	glm::vec3 position;
	glm::vec3 up;
};

// Made it a struct just in case we need to store something else
struct TrackCursor {
	unsigned int node = 0;
};

class TrackSegment {

private:

	std::vector<BezierKnot> knots;
	std::vector<TrackNode> nodes;

	void build(int samplesPerCurve);
	
public:

	TrackSegment(std::vector<BezierKnot> knots, int samplesPerCurve = 32);
	static TrackSegment fromFile(const std::string& path, int samplesPerCurve = 32);

	const std::vector<TrackNode>& getNodes() const { return nodes; }
	
	TrackNode evaluateBezierPoint(const BezierKnot& a, const BezierKnot& b, float t);
	TrackNode applyParallelTransport(TrackNode current, TrackNode previous);
	TrackNode applyTilt(TrackNode current);
};

class TrackProfile {

private:

	std::vector<BezierKnot2D> knots;
	std::vector<ProfilePoint> points;
	bool loop = false;
	float width = 0.0f;

	void build(int samplesPerCurve);

public:

	TrackProfile() = default;
	TrackProfile(std::vector<BezierKnot2D> knots, bool loop, int samplesPerCurve);
	static TrackProfile fromFile(const std::string& path, int samplesPerCurve);

	const std::vector<ProfilePoint>& getPoints() const { return points; }
	bool isLoop() const { return loop; }
	float getWidth() const { return width; }

	ProfilePoint evaluateBezierPoints(const BezierKnot2D& a, const BezierKnot2D& b, float t);
	ProfileContact getProfileContact(glm::vec2 point) const;
};

class TrackMesh {

private:

	std::vector<TrackVertex> vertices;
	std::vector<unsigned int> indices;

public:

	TrackMesh(const std::vector<TrackNode>& nodes, const TrackProfile& profile);

	const std::vector<TrackVertex>& getVertices() const { return vertices; };
	const std::vector<unsigned int>& getIndices() const { return indices; };
};

class Track {

private:
	std::vector<TrackNode> nodes;
	TrackProfile profile;

public:

	Track() = default;
	Track(const TrackSegment& segment, TrackProfile profile);

	const std::vector<TrackNode>& getNodes() const { return nodes; }
	const TrackProfile& getProfile() const { return profile; }

	TrackContact locate(const glm::vec3& worldPosition, TrackCursor& cursor) const;
	FindVertexResult find_vertex(const glm::vec3& worldPosition, TrackCursor& cursor) const;

	std::tuple<std::vector<float>, std::vector<unsigned int>> mesh() const;	
};