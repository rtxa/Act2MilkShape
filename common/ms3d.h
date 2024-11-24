//
//                MilkShape 3D 1.8.5 File Format Specification
//
//                  This specification is written in C style.
//
// The data structures are defined in the order as they appear in the .ms3d
// file.
//

#pragma once

#include <cstdint>
#include <vector>

#pragma pack(1)
struct ms3d_header_t {
  char id[10];  // always "MS3D000000"
  int version;  // 4
};

struct ms3d_vertex_t {
  uint8_t flags;           // Selected | HIDDEN
  float vertex[3];         // Position
  char boneId;             // Bone index or -1
  uint8_t referenceCount;  // Reference count
};

struct ms3d_triangle_t {
  uint16_t flags;             // SELECTED | SELECTED2 | HIDDEN
  uint16_t vertexIndices[3];  // Indices of vertices
  float vertexNormals[3][3];  // Normals for each vertex
  float s[3];                 // Texture coordinates (u)
  float t[3];                 // Texture coordinates (v)
  uint8_t smoothingGroup;     // Smoothing group: 1 - 32
  uint8_t groupIndex;         // Material index
};

struct ms3d_material_t {
  char name[32];       // Material name
  float ambient[4];    // Ambient color
  float diffuse[4];    // Diffuse color
  float specular[4];   // Specular color
  float emissive[4];   // Emissive color
  float shininess;     // Shininess factor 0.0f - 128.0f
  float transparency;  // Transparency 0.0f - 1.0f
  char mode;           // Material mode - 0, 1, 2 is unused now
  char texture[128];   // Texture file name
  char alphamap[128];  // Alpha map file name
};

struct ms3d_joint_t {
  uint8_t flags;               // SELECTED | DIRTY
  char name[32];               // Joint name
  char parentName[32];         // Parent joint name
  float rotation[3];           // Local rotation
  float position[3];           // Local position
  uint16_t numKeyFramesRot;    // Number of rotation keyframes
  uint16_t numKeyFramesTrans;  // Number of translation keyframes

  // Local animation matrices
  // ms3d_keyframe_rot keyFramesRot[numKeyFramesRot];

  // Local animation matrices
  // ms3d_keyframe_pos keyFramesTrans[numKeyFramesTrans];
};

struct ms3d_group_t {
  uint8_t flags{0};                       // SELECTED | HIDDEN
  char name[32];                          //
  uint16_t numtriangles{0};               //
  std::vector<uint16_t> triangleIndices;  // Dynamic
  char materialIndex{0};                  // -1 = no material
};

struct ms3d_anim_controls_t {
  float animationFPS;
  float currentTime;
  uint32_t totalFrames;
};

struct ms3d_model_t {
  ms3d_header_t header;
  std::vector<ms3d_vertex_t> vertices;
  std::vector<ms3d_triangle_t> triangles;
  std::vector<ms3d_group_t> groups;
  std::vector<ms3d_material_t> materials;
  ms3d_anim_controls_t animControls;
  std::vector<ms3d_joint_t> joints;

  ms3d_model_t(int vertexCount,
               int triangleCount,
               int materialCount,
               int jointCount)
      : vertices(vertexCount),
        triangles(triangleCount),
        materials(materialCount),
        joints(jointCount) {}
};

#pragma pack()
