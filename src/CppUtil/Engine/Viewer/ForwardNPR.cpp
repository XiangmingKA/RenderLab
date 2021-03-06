#include <CppUtil/Engine/ForwardNPR.h>

#include <CppUtil/Engine/AllBSDFs.h>

#include <CppUtil/Engine/Gooch.h>

#include <CppUtil/Qt/RawAPI_OGLW.h>
#include <CppUtil/Qt/RawAPI_Define.h>

#include <CppUtil/OpenGL/CommonDefine.h>
#include <CppUtil/OpenGL/Texture.h>
#include <CppUtil/OpenGL/Shader.h>

#include <CppUtil/Basic/Image.h>

#include <ROOT_PATH.h>

using namespace CppUtil::Engine;
using namespace CppUtil::QT;
using namespace CppUtil::OpenGL;
using namespace CppUtil::Basic;
using namespace CppUtil;
using namespace Define;
using namespace std;

void ForwardNPR::Init() {
	ForwardRaster::Init();

	InitShaders();
}

void ForwardNPR::InitShaders() {
	InitShaderGooch();
}

void ForwardNPR::InitShaderGooch() {
	shaderGooch = Shader(ROOT_PATH + str_Basic_P3N3T2T3_vs, ROOT_PATH + "data/shaders/Engine/Gooch.fs");

	shaderGooch.SetInt("gooch.colorTexture", 0);

	RegShader(shaderGooch, 1);
}

void ForwardNPR::Visit(Ptr<Gooch> gooch) {
	SetCurShader(shaderGooch);

	shaderGooch.SetVec3f("gooch.colorFactor", gooch->colorFactor);
	if (gooch->colorTexture && gooch->colorTexture->IsValid()) {
		shaderGooch.SetBool("gooch.haveColorTexture", true);
		pOGLW->GetTex(gooch->colorTexture).Use(0);
	}
	else
		shaderGooch.SetBool("gooch.haveColorTexture", false);

	UseLightTex(shaderGooch);
}
