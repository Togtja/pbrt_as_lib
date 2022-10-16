#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <string_view>

#include "core/api.h"
#include "core/paramset.h"

using namespace std::string_literals;

// CONFIGURATION:
constexpr int RAY_DEPTH             = 5;  // 10
constexpr int RAYS                  = 16; // 128
constexpr int FOV                   = 90;
constexpr int XRES                  = 1000;
constexpr int YRES                  = 500;
constexpr std::string_view FILENAME = "dusk.exr";

template<class T>
std::unique_ptr<T[]> makeSingle(const T& val) {
    auto ptr = std::make_unique<T[]>(1);
    ptr[0]   = val;
    return std::move(ptr);
}

template<class T>
std::unique_ptr<T[]> makeMulti(std::vector<T> vals) {
    auto ptr = std::make_unique<T[]>(vals.size());
    int i    = 0;
    for (const auto& v : vals) {
        ptr[i++] = v;
    }
    return std::move(ptr);
}

void add_material(const std::string name, const std::string& filename) {
    // Autumn Leaves Materials
    pbrt::ParamSet TextureParam;
    TextureParam.AddString("filename", makeSingle(filename), 1);
    TextureParam.AddFloat("uscale", makeSingle(pbrt::Float(1)));
    TextureParam.AddFloat("vscale", makeSingle(pbrt::Float(1)));
    pbrt::pbrtTexture(name, "spectrum", "imagemap", TextureParam);
    pbrt::ParamSet MaterialParam;
    MaterialParam.AddString("type", makeSingle("matte"s), 1);
    MaterialParam.AddTexture("Kd", name);
    pbrt::pbrtMakeNamedMaterial(name, MaterialParam);
}

struct Transformation {
    pbrt::Vector3f Translate{0, 0, 0};
    pbrt::Vector3f Scale{1, 1, 1};

    pbrt::Float Deg = 0.0;
    pbrt::Vector3f Rotate{1, 0, 0};

    std::string to_string() const {
        std::stringstream ss;
        ss << "Translate " << Translate.x << " " << Translate.y << " " << Translate.z << std::endl;
        ss << "Scale " << Scale.x << " " << Scale.y << " " << Scale.z << std::endl;
        ss << "Rotate " << Deg << " " << Rotate.x << " " << Rotate.y << " " << Rotate.z << std::endl;
        return ss.str();
    }

    void print() const { std::cout << to_string(); }
};

void add_attribute(const Transformation& transformation, const std::string& material, const std::string& include = "") {
    pbrt::pbrtAttributeBegin();

    pbrt::pbrtTranslate(transformation.Translate.x, transformation.Translate.y, transformation.Translate.z);
    pbrt::pbrtScale(transformation.Scale.x, transformation.Scale.y, transformation.Scale.z);
    pbrt::pbrtRotate(transformation.Deg, transformation.Rotate.x, transformation.Rotate.y, transformation.Rotate.z);
    if (material == ".pbrt") {
        pbrt::pbrtParseFile(material);
    }
    else {
        pbrt::pbrtNamedMaterial(material);
    }

    pbrt::pbrtParseFile(include);

    pbrt::pbrtAttributeEnd();
}

void create_object(const std::string& name,
                   std::vector<std::string> materials,
                   std::vector<Transformation> transformations,
                   std::vector<std::string> includes) {
    pbrt::pbrtObjectBegin(name);
    if (materials.size() != transformations.size() && materials.size() != includes.size()) {
        return;
    }
    for (int i = 0; i < materials.size(); i++) {
        add_attribute(transformations[i], materials[i], includes[i]);
    }
    pbrt::pbrtObjectEnd();
}

void add_object(const std::string& name, const Transformation& transformation) {
    pbrt::pbrtAttributeBegin();
    pbrt::pbrtTranslate(transformation.Translate.x, transformation.Translate.y, transformation.Translate.z);
    pbrt::pbrtScale(transformation.Scale.x, transformation.Scale.y, transformation.Scale.z);
    pbrt::pbrtRotate(transformation.Deg, transformation.Rotate.x, transformation.Rotate.y, transformation.Rotate.z);
    pbrt::pbrtObjectInstance(name);
    pbrt::pbrtAttributeEnd();
}

auto matrixToAxisAngle(const float m[][4]) {
    struct ret {
        float deg{};
        pbrt::Vector3f vec{};
    };
    // To quaternion
    float qw, qx, qy, qz;
    qw = sqrt(std::max(0.f, 1 + m[0][0] + m[1][1] + m[2][2])) / 2;
    qx = sqrt(std::max(0.f, 1 + m[0][0] - m[1][1] - m[2][2])) / 2;
    qy = sqrt(std::max(0.f, 1 - m[0][0] + m[1][1] - m[2][2])) / 2;
    qz = sqrt(std::max(0.f, 1 - m[0][0] - m[1][1] + m[2][2])) / 2;

    // To angle axis
    float deg = (2 * acos(qw) * (180.f / pbrt::Pi));
    float x   = qx / sqrt(1 - qw * qw);
    float y   = qy / sqrt(1 - qw * qw);
    float z   = qz / sqrt(1 - qw * qw);

    return ret{deg, {x, y, z}};
}

int main(int argc, char** argv) {
    
    std::cout << "Hello world!" << std::endl;
        pbrt::Options opt;
    opt.nThreads = std::thread::hardware_concurrency();
    pbrt::pbrtInit(opt);
  pbrt::pbrtLookAt(/*E*/ 100, 0, 0, /*L*/ 0, 0, 0, /*U*/ 0, 0, 1);

    // ***** Camera *****
    pbrt::ParamSet cameraParams;
    cameraParams.AddFloat("fov", makeSingle(pbrt::Float(FOV)), 1);

    pbrt::pbrtCamera("perspective", cameraParams);

    // ***** Film *****
    pbrt::ParamSet filmParams;
    // xresolution
    filmParams.AddInt("xresolution", makeSingle(XRES), 1);
    // yresolution
    filmParams.AddInt("yresolution", makeSingle(YRES), 1);
    // filename
    filmParams.AddString("filename", makeSingle(std::string(FILENAME)), 1);

    pbrt::pbrtFilm("image", filmParams);

    // ***** Sampler *****
    pbrt::ParamSet samplerParams;
    samplerParams.AddInt("pixelsamples", makeSingle(RAYS), 1);
    pbrt::pbrtSampler("halton", samplerParams);

    // ***** Integrator *****
    pbrt::ParamSet integratorParams;
    integratorParams.AddInt("maxdepth", makeSingle(RAY_DEPTH), 1);

    pbrt::pbrtIntegrator("path", integratorParams);

    // ***** Animation Time *****
    pbrt::pbrtTransformTimes(0, 1);

    pbrt::pbrtWorldBegin();

    // ***** Light *****
    pbrt::pbrtAttributeBegin();
    {
        pbrt::pbrtRotate(320, 0, 0, 1);
        pbrt::pbrtTranslate(0, 0, -50);

        pbrt::ParamSet lightSrc;
        lightSrc.AddString("mapname", makeSingle("textures/skylight-dusk.exr"s), 1);
        pbrt::pbrtLightSource("infinite", lightSrc);
    }
    pbrt::pbrtAttributeEnd();

    // ***** Materials *****
    constexpr std::string_view LEAVES_AUTUMN = "Leaves-autumn";
    constexpr std::string_view BARK          = "Bark";
    constexpr std::string_view LEAVES_PINE   = "Leaves-pine";
    // Autumn Leaves Materials
    add_material(LEAVES_AUTUMN.data(), "./treeTexture/Leaves-2.png");
    add_material(BARK.data(), "./treeTexture/Bark-4.png");
    add_material(LEAVES_PINE.data(), "./treeTexture/Leaves-4.png");

    // ***** Objects *****

    // MAPLE_TREE
    Transformation transfLeaves_Autumn = {
        pbrt::Vector3f{0, 0, -2}, // Translate
        pbrt::Vector3f{5, 5, 5},  // Scale
        90,                       // Degrees
        pbrt::Vector3f{1, 0, 0},  // Rotate what plane the degrees

    };
    std::vector<Transformation> mapleTreeTrans  = {transfLeaves_Autumn, transfLeaves_Autumn};
    std::vector<std::string> mapleTreeMaterials = {LEAVES_AUTUMN.data(), BARK.data()};
    std::vector<std::string> mapleTreeIncludes  = {"./treegeometry/mapleTree-leaves.pbrt", "./treegeometry/mapleTree-trunk.pbrt"};

    const std::string MAPLE_TREE = "mapleTree";
    create_object(MAPLE_TREE, mapleTreeMaterials, mapleTreeTrans, mapleTreeIncludes);
    // PINE_TREE
    Transformation transfLeaves_Pine = {
        pbrt::Vector3f{0, 0, -2}, // Translate
        pbrt::Vector3f{4, 4, 4},  // Scale
        90,                       // Degrees
        pbrt::Vector3f{1, 0, 0},  // Rotate what plane the degrees

    };
    std::vector<Transformation> pineTreeTrans  = {transfLeaves_Pine, transfLeaves_Pine};
    std::vector<std::string> pineTreeMaterials = {LEAVES_PINE.data(), BARK.data()};
    std::vector<std::string> pineTreeIncludes  = {"./treegeometry/pineTree-leaves.pbrt", "./treegeometry/pineTree-trunk.pbrt"};

    const std::string PINE_TREE = "pineTree";
    create_object(PINE_TREE, pineTreeMaterials, pineTreeTrans, pineTreeIncludes);

    // ***** POPULATE THE SCENE *****
    Transformation t{};
    t.Translate.y = 75;
    t.Translate.x = -10;
    add_object(MAPLE_TREE, t);
    t.Translate.y = 120;
    add_object(MAPLE_TREE, t);
    t.Translate.y = 175;
    add_object(MAPLE_TREE, t);

    // PINE_TREES
    int pineTree_nr = 10;
    //  Transformation: Translate{x,y,z}, Scale{x,y,x}, angle, rotate{x,y,z}
    Transformation pineTransform{{-20, -120, 0}, {0.8, 0.8, 0.8}};
    float ypush = 40;
    for (int i = 0; i < pineTree_nr; i++) {
        add_object(PINE_TREE, pineTransform);
        pineTransform.Scale *= 0.8;
        ypush *= 0.70;
        pineTransform.Translate.y += ypush;
        pineTransform.Translate.x -= 8;
    }

    pbrt::pbrtWorldEnd();
    pbrt::pbrtCleanup();
    return 0;
}
