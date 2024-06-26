cmake_minimum_required(VERSION 3.5)

set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
set(CMAKE_PREFIX_PATH $ENV{VCPKG_ROOT}/installed/x64-windows/share)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

cmake_host_system_information(RESULT CMake_MSVC_PARALLEL QUERY NUMBER_OF_PHYSICAL_CORES)

if(CMake_MSVC_PARALLEL)
  if(CMake_MSVC_PARALLEL GREATER 0)
    message(${CMake_MSVC_PARALLEL})
    add_definitions(/MP${CMake_MSVC_PARALLEL})
  else()
    add_definitions(/MP)
  endif()
endif()

project(LAKOT)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(LAKOT_USE_GPU "Use GPU" OFF)
if (LAKOT_USE_GPU)
    add_definitions(-DLAKOT_USE_GPU)
endif()

set(SRC_DIRECTORY ${CMAKE_SOURCE_DIR}/src)
    set(APPLICATION_DIRECTORY ${SRC_DIRECTORY}/application)
        set(GRAPHICS_DIRECTORY ${APPLICATION_DIRECTORY}/graphics)
            set(GUI_DIRECTORY ${GRAPHICS_DIRECTORY}/gui)
                set(ELEMENTS_DIRECTORY ${GUI_DIRECTORY}/elements)
            set(MODEL_DIRECTORY ${GRAPHICS_DIRECTORY}/model)
	    set(PARTICLE_DIRECTORY ${GRAPHICS_DIRECTORY}/particle)
            set(RENDER_DIRECTORY ${GRAPHICS_DIRECTORY}/render)
            set(SCENE_DIRECTORY ${GRAPHICS_DIRECTORY}/scene)
            # set(SHADER_DIRECTORY ${GRAPHICS_DIRECTORY}/shader)
    set(CORE_DIRECTORY ${SRC_DIRECTORY}/core)    
set(THIRD_PARTY_DIRECTORY ${CMAKE_SOURCE_DIR}/third_party)

add_subdirectory(${CORE_DIRECTORY})

set(SRC_FILES
    ${SRC_DIRECTORY}/main.cpp

    ${SRC_DIRECTORY}/Lakot.h
    ${SRC_DIRECTORY}/Lakot.cpp
)
    set(APPLICATION_FILES
	${APPLICATION_DIRECTORY}/Application.h
	${APPLICATION_DIRECTORY}/Application.cpp

	${APPLICATION_DIRECTORY}/Settings.h
	${APPLICATION_DIRECTORY}/Settings.cpp
    )

	set(GRAPHICS_FILES

	)

            set(GUI_FILES
                ${GUI_DIRECTORY}/GUI.h
                ${GUI_DIRECTORY}/GUI.cpp
            )

                set(ELEMENTS_FILES
                    ${ELEMENTS_DIRECTORY}/CameraElement.h
                    ${ELEMENTS_DIRECTORY}/CameraElement.cpp

                    ${ELEMENTS_DIRECTORY}/MainMenuBarElement.h
                    ${ELEMENTS_DIRECTORY}/MainMenuBarElement.cpp
                )

	    set(MODEL_FILES
		${MODEL_DIRECTORY}/Animation.h
		${MODEL_DIRECTORY}/Animation.cpp

    #	    ${MODEL_DIRECTORY}/Animator.h
    #	    ${MODEL_DIRECTORY}/Animator.cpp

		${MODEL_DIRECTORY}/Bone.h
		${MODEL_DIRECTORY}/Bone.cpp

		${MODEL_DIRECTORY}/KeyFrameChannel.h
		${MODEL_DIRECTORY}/KeyFrameChannel.cpp

		${MODEL_DIRECTORY}/Material.h
		${MODEL_DIRECTORY}/Material.cpp

		${MODEL_DIRECTORY}/Mesh.h
		${MODEL_DIRECTORY}/Mesh.cpp

		${MODEL_DIRECTORY}/Model.h
		${MODEL_DIRECTORY}/Model.cpp

		${MODEL_DIRECTORY}/Node.h
		${MODEL_DIRECTORY}/Node.cpp

		${MODEL_DIRECTORY}/ModelFactory.h
		${MODEL_DIRECTORY}/ModelFactory.cpp

		${MODEL_DIRECTORY}/Texture.h
		${MODEL_DIRECTORY}/Texture.cpp
	    )

	    set(PARTICLE_FILES
		${PARTICLE_DIRECTORY}/Particle.h
		${PARTICLE_DIRECTORY}/Particle.cpp

		${PARTICLE_DIRECTORY}/ParticleManager.h
		${PARTICLE_DIRECTORY}/ParticleManager.cpp

		${PARTICLE_DIRECTORY}/ParticleProvider.h
		${PARTICLE_DIRECTORY}/ParticleProvider.cpp

                ${PARTICLE_DIRECTORY}/ParticleRenderer.h
                ${PARTICLE_DIRECTORY}/ParticleRenderer.cpp
            )

	    set(RENDER_FILES
		${RENDER_DIRECTORY}/RenderManager.h
		${RENDER_DIRECTORY}/RenderManager.cpp
	    )

            set(SCENE_FILES
                ${SCENE_DIRECTORY}/Projection.h
                ${SCENE_DIRECTORY}/Projection.cpp

                ${SCENE_DIRECTORY}/Scene.h
                ${SCENE_DIRECTORY}/Scene.cpp
            )

         #    set(SHADER_FILES
                # ${SHADER_DIRECTORY}/ShaderModel.h
                # ${SHADER_DIRECTORY}/ShaderModel.cpp

                # ${SHADER_DIRECTORY}/ShaderParticle.h
                # ${SHADER_DIRECTORY}/ShaderParticle.cpp
         #    )

include_directories(
    ${SRC_DIRECTORY}
    $ENV{VCPKG_ROOT}/installed/x64-windows/include
)

if(CMAKE_BUILD_TYPE EQUAL "DEBUG")
    link_directories(
        $ENV{VCPKG_ROOT}/installed/x64-windows/debug/lib
    )
else()
    link_directories(
        $ENV{VCPKG_ROOT}/installed/x64-windows/lib
    )
endif()

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

add_executable(${CMAKE_PROJECT_NAME}
    ${SRC_FILES}
	${APPLICATION_FILES}
            ${GRAPHICS_FILES}
                ${GUI_FILES}
                    ${ELEMENTS_FILES}
		${MODEL_FILES}
		${PARTICLE_FILES}
		${RENDER_FILES}
                ${SCENE_FILES}
                # ${SHADER_FILES}
                src/application/graphics/PostProcess.h src/application/graphics/PostProcess.cpp
                src/application/graphics/PixelReader.h src/application/graphics/PixelReader.cpp

)

find_package(spdlog CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

target_link_libraries(${CMAKE_PROJECT_NAME} 
    LAKOT_CORE_LIB
    ${OPENGL_LIBRARIES}
    spdlog::spdlog
    imgui::imgui
)
