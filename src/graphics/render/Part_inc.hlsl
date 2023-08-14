//
// Model Part API
//
#ifndef PART_INC
#define PART_INC

struct Part {
    uint numIndices;
    uint indexOffset;
    uint vertexOffset;
    uint attribOffset;
    uint material;
    uint numEdges;
    uint edgeOffset;
    uint skinOffset;
};

#endif
