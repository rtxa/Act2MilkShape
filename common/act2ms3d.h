/**
 * Act2MilkShape
 * Simple converter from Genesis3D .act to MilkShape .ms3d
 *
 * Copyright (c) 2024 rtxa
 * This code is licensed under the MIT license (see LICENSE.txt for details).
 */

#pragma once

#include "body_private.h"
#include "ms3d.h"

namespace act2ms3d {

const char* GENESISCC geStrBlock_GetString(const geStrBlock* SB, int Index) {
  assert(SB != NULL);
  assert(Index >= 0);
  assert(Index < SB->Count);
  assert(SB->SanityCheck == SB);
  return &(SB->Data.CharArray[SB->Data.IntArray[Index]]);
}

template <typename T>
void WriteToFile(std::ofstream& file, const T& data) {
  file.write(reinterpret_cast<const char*>(&data), sizeof(T));
}

template <typename T, typename SizeType>
void WriteVectorToFile(std::ofstream& file, const std::vector<T>& data) {
  SizeType count = static_cast<SizeType>(data.size());
  WriteToFile(file, count);  // Write the count first
  file.write(reinterpret_cast<const char*>(data.data()), sizeof(T) * count);
}

void WriteMS3DFile(const std::string& filename, const ms3d_model_t& model) {
  std::ofstream file(filename, std::ios::binary);
  if (!file) {
    std::cerr << "Failed to open file for writing: " << filename << std::endl;
    return;
  }

  // 1. Write the header
  WriteToFile(file, model.header);

  // 2. Write vertices
  WriteVectorToFile<ms3d_vertex_t, uint16_t>(file, model.vertices);

  // 3. Write triangles
  WriteVectorToFile<ms3d_triangle_t, uint16_t>(file, model.triangles);

  // 4. Write groups
  auto groupCount = static_cast<uint16_t>(model.groups.size());
  WriteToFile(file, groupCount);
  for (const auto& group : model.groups) {
    WriteToFile(file, group.flags);
    file.write(group.name, sizeof(char) * 32);
    WriteToFile(file, group.numtriangles);
    for (uint16_t index : group.triangleIndices) {
      WriteToFile(file, index);
    }
    WriteToFile(file, group.materialIndex);
  }

  // 5. Write materials
  WriteVectorToFile<ms3d_material_t, uint16_t>(file, model.materials);

  // 6. Write editor animation controls
  WriteToFile(file, model.animControls);

  // 7. Write joints
  WriteVectorToFile<ms3d_joint_t, uint16_t>(file, model.joints);

  file.close();
}

std::vector<std::string> GetMaterialNames(geStrBlock* materialNames) {
  if (!materialNames)
    return {};  // Check for null pointer
  std::vector<std::string> names;
  for (int i = 0; i < materialNames->Count; ++i) {
    int offset = materialNames->Data.IntArray[i];
    const char* name = &materialNames->Data.CharArray[offset];
    names.push_back(name);
  }
  return names;
}

// Function to recursively get the global transformation matrix for a bone
void GetGlobalTransformationMatrix(int boneIndex,
                                   geXForm3d* globalMatrix,
                                   const geBody* body) {
  if (boneIndex == -1)
    return;  // No bone index

  geXForm3d localMatrix = body->BoneArray[boneIndex].AttachmentMatrix;

  // If the bone has a parent, get its global transformation
  int parentBoneIndex =
      body->BoneArray[boneIndex]
          .ParentBoneIndex;  // Assuming you have a way to get the parent index
  if (parentBoneIndex != -1) {
    geXForm3d parentGlobal;
    GetGlobalTransformationMatrix(parentBoneIndex, &parentGlobal, body);
    geXForm3d_Multiply(&parentGlobal, &localMatrix, globalMatrix);
  } else {
    // If no parent, the global matrix is just the local one
    *globalMatrix = localMatrix;
  }
}

void QuaternionToEuler(const geQuaternion* q, geVec3d* angles) {
  constexpr auto M_PI = 3.14159265358979323846;

  // Roll (X)
  double sinr_cosp = 2.0 * (q->W * q->X + q->Y * q->Z);
  double cosr_cosp = 1.0 - 2.0 * (q->X * q->X + q->Y * q->Y);
  double roll = atan2(sinr_cosp, cosr_cosp);

  // Pitch (Y)
  double sinp = 2.0 * (q->W * q->Y - q->Z * q->X);
  double pitch;
  if (fabs(sinp) >= 1.0) {
    pitch = copysign(M_PI / 2, sinp);  // Use 90 degrees if out of range
  } else {
    pitch = asin(sinp);
  }

  // Yaw (Z)
  double siny_cosp = 2.0 * (q->W * q->Z + q->X * q->Y);
  double cosy_cosp = 1.0 - 2.0 * (q->Y * q->Y + q->Z * q->Z);
  double yaw = atan2(siny_cosp, cosy_cosp);

  // Convert to float and assign
  angles->X = static_cast<float>(roll);
  angles->Y = static_cast<float>(pitch);
  angles->Z = static_cast<float>(yaw);
}

void ConvertGeBodyToMS3D(const geBody* body, ms3d_model_t& model) {
  // 1. Fill header
  strcpy(model.header.id, "MS3D000000");
  model.header.version = 4;  // MS3D version

  // 2. Fill vertices
  for (int i = 0; i < body->XSkinVertexCount; ++i) {
    model.vertices[i].flags = 0;  // Default flags

    // Get the bone index for this vertex
    int boneIndex = body->XSkinVertexArray[i].BoneIndex;

    // Retrieve the global transformation matrix for the corresponding bone
    geXForm3d globalMatrix;
    GetGlobalTransformationMatrix(boneIndex, &globalMatrix, body);

    // Create the original vertex vector
    geVec3d originalVertex = body->XSkinVertexArray[i].XPoint;

    // Transform the vertex position using the global transformation matrix
    geVec3d transformedVertex;
    geXForm3d_Transform(&globalMatrix, &originalVertex, &transformedVertex);

    // Set the transformed vertex position
    model.vertices[i].vertex[0] = transformedVertex.X;
    model.vertices[i].vertex[1] = transformedVertex.Y;
    model.vertices[i].vertex[2] = transformedVertex.Z;

    model.vertices[i].boneId = boneIndex;
    model.vertices[i].referenceCount = 0;
  }

  // 3. Fill triangles
  for (int i = 0; i < body->SkinFaces[0].FaceCount; ++i) {
    model.triangles[i].flags = 0;
    const auto& triangle = body->SkinFaces[0].FaceArray[i];

    model.triangles[i].vertexIndices[0] = triangle.VtxIndex[0];
    model.triangles[i].vertexIndices[1] = triangle.VtxIndex[1];
    model.triangles[i].vertexIndices[2] = triangle.VtxIndex[2];

    for (int j = 0; j < 3; ++j) {
      int normalIndex = triangle.NormalIndex[j];
      model.triangles[i].vertexNormals[j][0] =
          body->SkinNormalArray[normalIndex].Normal.X;
      model.triangles[i].vertexNormals[j][1] =
          body->SkinNormalArray[normalIndex].Normal.Y;
      model.triangles[i].vertexNormals[j][2] =
          body->SkinNormalArray[normalIndex].Normal.Z;

      int vtxIndex = triangle.VtxIndex[j];
      model.triangles[i].s[j] = body->XSkinVertexArray[vtxIndex].XU;
      model.triangles[i].t[j] = body->XSkinVertexArray[vtxIndex].XV;
    }

    model.triangles[i].smoothingGroup = 1;  // Default smoothing group
    model.triangles[i].groupIndex = 0;      // Default group index
  }

  // 4. Fill groups based on MaterialIndex
  std::vector<ms3d_group_t> groupList;
  ms3d_group_t currentGroup{};
  int numGlobalTriangles = 0;

  // Initialize the first group
  currentGroup.materialIndex =
      static_cast<char>(body->SkinFaces[0].FaceArray[0].MaterialIndex);
  sprintf(currentGroup.name, "Group %d", groupList.size() + 1);

  for (int i = 0; i < body->SkinFaces[0].FaceCount; ++i) {
    const auto& triangle = body->SkinFaces[0].FaceArray[i];

    // Check if we need to start a new group
    if (triangle.MaterialIndex != currentGroup.materialIndex) {
      groupList.push_back(currentGroup);
      currentGroup = {};  // Clear for the next group
      currentGroup.materialIndex = static_cast<char>(triangle.MaterialIndex);
      sprintf(currentGroup.name, "Group %d", groupList.size() + 1);
    }

    currentGroup.triangleIndices.push_back(numGlobalTriangles++);
    currentGroup.numtriangles++;
  }

  // Push the last group if it contains triangles
  if (currentGroup.numtriangles > 0) {
    groupList.push_back(currentGroup);
  }
  model.groups = groupList;

  // 5. Fill materials
  std::vector<std::string> names = GetMaterialNames(body->MaterialNames);
  for (int i = 0; i < body->MaterialCount; ++i) {
    auto& mat = model.materials[i];
    mat.ambient[0] = mat.ambient[1] = mat.ambient[2] = mat.ambient[3] = 1.0f;
    mat.diffuse[0] = mat.diffuse[1] = mat.diffuse[2] = mat.diffuse[3] = 1.0f;
    mat.specular[0] = mat.specular[1] = mat.specular[2] = 0.0f;
    mat.specular[3] = 1.0f;
    mat.emissive[0] = mat.emissive[1] = mat.emissive[2] = 0.0f;
    mat.emissive[3] = 1.0f;
    mat.shininess = 0.0f;
    mat.transparency = 1.0f;
    mat.mode = 2;

    sprintf(mat.name, "%s", names[i].c_str());
    sprintf(mat.texture, "%s.bmp", names[i].c_str());
    mat.alphamap[0] = '\x00';
  }

  // 6. Set editor animation controls
  model.animControls.animationFPS = 0.0f;
  model.animControls.currentTime = 0.0f;
  model.animControls.totalFrames = 0;

  // 7. Fill joints
  for (int i = 0; i < body->BoneCount; ++i) {
    model.joints[i].flags = 0;
    geBody_Bone* bone = &body->BoneArray[i];

    // Set joint names
    strcpy(model.joints[i].name, geStrBlock_GetString(body->BoneNames, i));
    if (bone->ParentBoneIndex == -1) {
      model.joints[i].parentName[0] = '\x00';  // No parent
    } else {
      strcpy(model.joints[i].parentName,
             geStrBlock_GetString(body->BoneNames, bone->ParentBoneIndex));
    }

    // Set translation from the attachment matrix
    model.joints[i].position[0] = bone->AttachmentMatrix.Translation.X;
    model.joints[i].position[1] = bone->AttachmentMatrix.Translation.Y;
    model.joints[i].position[2] = bone->AttachmentMatrix.Translation.Z;

    // Extract rotation from the attachment matrix
    geQuaternion quaternion;
    geQuaternion_FromMatrix(&bone->AttachmentMatrix, &quaternion);

    // Convert quaternion to Euler angles
    geVec3d angles;
    QuaternionToEuler(&quaternion, &angles);

    // Store Euler angles in the joint structure
    model.joints[i].rotation[0] = angles.X;
    model.joints[i].rotation[1] = angles.Y;
    model.joints[i].rotation[2] = angles.Z;

    // No animations for now
    model.joints[i].numKeyFramesRot = 0;
    model.joints[i].numKeyFramesTrans = 0;
  }
}
}  // namespace act2ms3d