cmake_minimum_required(VERSION 3.0)

# Create project
set(ProjectName "image2array")
project(${ProjectName})

# Create executable file
add_executable(${ProjectName} Main.cpp)

# For OpenCV
find_package(OpenCV REQUIRED)
target_include_directories(${ProjectName} PUBLIC ${OpenCV_INCLUDE_DIRS})
target_link_libraries(${ProjectName} ${OpenCV_LIBS})
