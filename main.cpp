/**
 * Act2MilkShape
 * Simple converter from Genesis3D .act to MilkShape .ms3d
 *
 * Copyright (c) 2024 rtxa
 * This code is licensed under the MIT license (see LICENSE.txt for details).
 */

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <GenesisSDK/actor.h>
#include <GenesisSDK/bitmap.h>
#include <GenesisSDK/genesis.h>

#include "common/act2ms3d.h"
#include "common/ms3d.h"

int main(int argc, char* argv[]) {
  std::string actorPath;
  std::string outputPath;

  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <input_file> [<output_file>]"
              << std::endl;
    return 1;
  }

  actorPath = argv[1];
  if (argc >= 3) {
    outputPath = argv[2];
  }

  auto* actorFile =
      geVFile_OpenNewSystem(nullptr, GE_VFILE_TYPE_DOS, actorPath.c_str(),
                            nullptr, GE_VFILE_OPEN_READONLY);
  if (!actorFile) {
    std::cout << "Failed to open actor file: " << actorPath << std::endl;
    return 1;
  }

  auto* actorDef = geActor_DefCreateFromFile(actorFile);
  if (!actorDef) {
    std::cout << "Failed to create definition from actor file" << std::endl;
    return 1;
  }

  geVFile_Close(actorFile);

  auto* body = geActor_GetBody(actorDef);

  int vertexCount = body->XSkinVertexCount;
  int triangleCount = body->SkinFaces[0].FaceCount;
  int materialCount = body->MaterialCount;
  int jointCount = body->BoneCount;

  ms3d_model_t model{vertexCount, triangleCount, materialCount, jointCount};
  act2ms3d::ConvertGeBodyToMS3D(body, model);

  if (outputPath.empty()) {
    auto path = std::filesystem::path(actorPath);
    outputPath =
        (path.parent_path() / ("output_" + path.stem().string() + ".ms3d"))
            .string();
  }

  act2ms3d::WriteMS3DFile(outputPath, model);

  geActor_DefDestroy(&actorDef);

  std::cout << "Conversion complete: " << outputPath << std::endl;

  return 0;
}
