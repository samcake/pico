
typedef int4 Edge; // x and y are vertex indices, z and w are triangle indices in index buffer
StructuredBuffer<Edge>  edge_array : register(t5);

bool mesh_edge_isAttribSeam(int4 edge) {
    return edge.w < 0;
}
int mesh_edge_triangle0(int4 edge) {
    return edge.z;
}
int mesh_edge_triangle1(int4 edge) {
    return (edge.w < 0 ? -edge.w - 1 : edge.w);
}

bool mesh_edge_isOutline(int4 edge) { // no c1 continuity neighbor face in the mesh
    return edge.z == mesh_edge_triangle1(edge);
}

typedef int4 FaceEdge; // x, y and z are the 3 edge indices of the face, allowing to retreive neighbor faces
StructuredBuffer<FaceEdge>  face_array : register(t6);


uint3 mesh_triangle_getAdjacentTriangles(uint triangleId, int edgeOffset) {
    int3 faceEdges = face_array[triangleId].xyz;

    uint3 triangles;
    for (int e = 0; e < 3; ++e) {
        Edge edge = edge_array[faceEdges[e] + edgeOffset];
        triangles[e] = ((edge.z == triangleId) ? mesh_edge_triangle1(edge) : mesh_edge_triangle0(edge));
    }
    return triangles;
}

int2 mesh_triangle_nextTriangleAroundFromAdjacent(int dir, uint pivotTriangle, uint fromTriangle, int edgeOffset) {

    uint3 pivotTriAdj = mesh_triangle_getAdjacentTriangles(pivotTriangle, edgeOffset);

    int adjEdgeId = (pivotTriAdj.x == fromTriangle ? 0 :
        (pivotTriAdj.y == fromTriangle ? 1 : 2));

    int nextEdgeId = (adjEdgeId + dir) % 3;

    int nextTri = pivotTriAdj[nextEdgeId];

    return int2(nextTri, nextEdgeId);
}

int mesh_triangle_detectAdjTriangle(uint nextRingTri, uint3 adjTriangles) {
    int detectAdjTriangle = -1 + 1 * (adjTriangles.x == nextRingTri) + 2 * (adjTriangles.y == nextRingTri) + 3 * (adjTriangles.z == nextRingTri);
    return detectAdjTriangle;
}

int mesh_triangle_gatherTriangleRing(out int ring[32], uint pivotTriangle, int edgeOffset) {
    const int MAX_RING_LENGTH = 32;
    uint3 adjFaces = mesh_triangle_getAdjacentTriangles(pivotTriangle, edgeOffset);

    int prevTri = pivotTriangle;
    int currentTri = adjFaces[0]; // find the other "ring" triangles from the last one
    ring[0] = currentTri;
    int ringLength = 1;

    int direction = +1;
    for (int rt = 1; rt < MAX_RING_LENGTH; ) {
        int2 next = mesh_triangle_nextTriangleAroundFromAdjacent(direction, currentTri, prevTri, edgeOffset);

        int detectAdjTriangle = mesh_triangle_detectAdjTriangle(next.x, adjFaces);
        prevTri = currentTri;
        currentTri = next.x;

        if (detectAdjTriangle == 0) {
            rt = MAX_RING_LENGTH;
        } else {
            // found the next triangle on the ring, not an adjTri
            ring[ringLength] = currentTri;
            ringLength++;
            rt++;

            // reached another adj triangle;
            if (detectAdjTriangle > 0) {
                // reset the search from the center triangle
                prevTri = pivotTriangle;
            }
        }
    }

    return ringLength;
}


int mesh_triangle_gatherTriangleRingSorted(out int ring[32], uint pivotTriangle, int edgeOffset) {
    const int MAX_RING_LENGTH = 32;
    uint3 adjFaces = mesh_triangle_getAdjacentTriangles(pivotTriangle, edgeOffset);
    ring[0] = pivotTriangle;
    ring[1] = adjFaces.x;
    ring[2] = adjFaces.y;
    ring[3] = adjFaces.z;
    int ringLength = 4;

    int prevTri = pivotTriangle;
    int currentTri = adjFaces[0]; // find the other "ring" triangles from the last one

    int direction = +1;
    for (int rt = 4; rt < MAX_RING_LENGTH; ) {
        int2 next = mesh_triangle_nextTriangleAroundFromAdjacent(direction, currentTri, prevTri, edgeOffset);

        int detectAdjTriangle = mesh_triangle_detectAdjTriangle(next.x, adjFaces);
        prevTri = currentTri;
        currentTri = next.x;

        if (detectAdjTriangle == 0) {
            rt = MAX_RING_LENGTH;
        } else {
            // reached another adj triangle;
            if (detectAdjTriangle > 0) {
                // reset the search from the center triangle
                prevTri = pivotTriangle;
            } else {
                // found the next triangle on the ring, not an adjTri
                ring[ringLength] = currentTri;
                ringLength++;
                rt++;
            }
        }
    }

    return ringLength;
}

