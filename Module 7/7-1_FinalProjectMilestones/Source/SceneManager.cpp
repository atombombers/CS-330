///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"
#include "ShapeMeshes.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// clear the allocated memory
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	// destroy the created OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// Prevent alignment crashes 
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

 /***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	// Load the chosen textures for the objects
	CreateGLTexture(
		"istockphoto-1328591896-612x612.jpg",
		"wood");

	CreateGLTexture(
		"2517642e-8e78-4f2e-a1e3-99e88ae43803.jpg",
		"metal");

	CreateGLTexture(
		"wall.jpg",
		"wall");

	CreateGLTexture(
		"window.jpg",
		"window");

	CreateGLTexture(
		"seamless-white-leather-texture-thumb30.jpg",
		"fabric");

	CreateGLTexture(
		"s_3378_1.jpg",
		"rug");

	CreateGLTexture(
		"360.jpg",
		"glass");

	CreateGLTexture(
		"wood.jpg",
		"whiteWood");

	CreateGLTexture(
		"360_F_59958917_1SvEPqvKnrGr68THJ35hqQUiHwQ6WhCN.jpg",
		"TV");


	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// -------------------- Lamp light --------------------

	m_pShaderManager->setVec3Value("lightSources[0].position", 8.5f, 3.1f, -5.5f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.08f, 0.07f, 0.06f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.65f, 0.55f, 0.40f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.15f, 0.13f, 0.10f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.12f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 18.0f);

	// -------------------- "window" light --------------------
	
	m_pShaderManager->setVec3Value("lightSources[1].position", 0.0f, 4.0f, -7.2f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.03f, 0.05f, 0.08f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.10f, 0.22f, 0.75f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.08f, 0.12f, 0.25f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.05f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 28.0f);
	
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the textures for the 3D scene
	LoadSceneTextures();

	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	// Floor
	m_basicMeshes->LoadPlaneMesh();

	// Lamp parts
	m_basicMeshes->LoadCylinderMesh();           // light center
	m_basicMeshes->LoadTaperedCylinderMesh();    // top + bottom flares

	m_basicMeshes->LoadBoxMesh(); // couch parts
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/****************************************************************/
	// -----------------******FLOOR******----------------------------
	/****************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.0f, 1.0f, 8.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Turn lighting ON for the floor
	m_pShaderManager->setBoolValue("bUseLighting", true);

	// Gives the floor a material with visible specular reflection
	m_pShaderManager->setVec3Value("material.ambientColor", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setFloatValue("material.ambientStrength", 0.25f);
	m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.12f, 0.10f, 0.08f));
	m_pShaderManager->setFloatValue("material.shininess", 30.0f);

	SetShaderColor(1, 1, 1, 1);

	// Turns on texturing + choose the texture
	SetShaderTexture("wood");

	SetTextureUVScale(1.0f, 1.0f);

	// Draws the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/
	// -----------------******WALL 1******------------------------ 
	//****************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.0f, 1.0f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 4.0f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Turn lighting ON for the floor
	m_pShaderManager->setBoolValue("bUseLighting", true);

	// Gives the floor a material with visible specular reflection
	m_pShaderManager->setVec3Value("material.ambientColor", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setFloatValue("material.ambientStrength", 0.25f);
	m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.12f, 0.10f, 0.08f));
	m_pShaderManager->setFloatValue("material.shininess", 30.0f);

	SetShaderColor(1, 1, 1, 1);

	// Turns on texturing + choose the texture
	SetShaderTexture("wall");

	SetTextureUVScale(2.0f, 1.0f);

	// Draws the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/
	// -----------------******WINDOW******--------------------------
	/****************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.0f, 1.0f, 3.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 4.0f, -7.99f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Turn lighting ON for the floor
	m_pShaderManager->setBoolValue("bUseLighting", true);

	// Gives the floor a material with visible specular reflection
	m_pShaderManager->setVec3Value("material.ambientColor", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setFloatValue("material.ambientStrength", 0.25f);
	m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.12f, 0.10f, 0.08f));
	m_pShaderManager->setFloatValue("material.shininess", 30.0f);

	SetShaderColor(1, 1, 1, 1);

	// Turns on texturing + choose the texture
	SetShaderTexture("window");

	SetTextureUVScale(1.0f, 1.0f);

	// Draws the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/
	// -----------------******WALL 2******---------------------------
	/****************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.0f, 1.0f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(10.0f, 4.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Turn lighting ON for the floor
	m_pShaderManager->setBoolValue("bUseLighting", true);

	// Gives the floor a material with visible specular reflection
	m_pShaderManager->setVec3Value("material.ambientColor", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setFloatValue("material.ambientStrength", 0.25f);
	m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.12f, 0.10f, 0.08f));
	m_pShaderManager->setFloatValue("material.shininess", 30.0f);

	SetShaderColor(1, 1, 1, 1);

	// Turns on texturing + choose the texture
	SetShaderTexture("wall");

	SetTextureUVScale(2.0f, 1.0f);

	// Draws the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/
	// -------------------******LAMP******---------------------------
	/****************************************************************/

	const float lampX = 8.5f; // Places the lamp near the right wall
	const float lampZ = -5.5f; // Moves the lamp slightly backwards
	const float baseY = 0.0f; // Top surface of the floor

	// ---------------------------------------------------------------
	// Bottom Tapered Cylinder
	// ---------------------------------------------------------------
	scaleXYZ = glm::vec3(0.85f, 2.50f, 0.85f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Half height placement so it sits on floor
	positionXYZ = glm::vec3(lampX, 0.0f, lampZ);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.45f, 0.56f, 0.60f, 1.0f);   // metallic gray

	SetShaderColor(1, 1, 1, 1);

	// turn on texturing + choose the texture
	SetShaderTexture("metal");

	m_basicMeshes->DrawTaperedCylinderMesh();

	// ---------------------------------------------------------------
	// Center Cylinder
	// ---------------------------------------------------------------
	scaleXYZ = glm::vec3(0.40f, 1.25f, 0.40f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Stack on top of base
	positionXYZ = glm::vec3(lampX, 2.50, lampZ);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	
	// MAKE IT GLOW

    // disable lighting so color is not affected by scene light
	m_pShaderManager->setBoolValue("bUseLighting", false);

	// disable texture so it doesn't darken the glow
	m_pShaderManager->setBoolValue("bUseTexture", false);

	// warm white emissive color (lamp shade)
	m_pShaderManager->setVec3Value("objectColor", 1.0f, 0.96f, 0.78f);

	m_basicMeshes->DrawCylinderMesh();

	// restore lighting for rest of scene
	m_pShaderManager->setBoolValue("bUseLighting", true);
	m_pShaderManager->setBoolValue("bUseTexture", true);

	// ---------------------------------------------------------------
	// Top Tapered Cylinder
	// ---------------------------------------------------------------
	scaleXYZ = glm::vec3(.80f, 2.00f, 0.80f);
	XrotationDegrees = 180.0f;   // flips taper so it widens upward
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Stack above glowing column
	positionXYZ = glm::vec3(lampX, 5.75f, lampZ);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.45f, 0.56f, 0.60f, 1.0f);

	SetShaderColor(1, 1, 1, 1);

	// turn on texturing + choose the texture
	SetShaderTexture("metal");

	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************************************************************/
	// -------------------******COUCH******--------------------------
	/****************************************************************/
	
	// ---------------------------------------------------------------
	// Couch Base
	// --------------------------------------------------------------- 
	scaleXYZ = glm::vec3(9.00f, 3.25f, 0.80f);
	XrotationDegrees = 90.0f;   
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-3.0f, 0.75f, -5.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	SetShaderColor(1, 1, 1, 1);

	// turn on texturing + choose the texture
	SetShaderTexture("fabric");

	m_basicMeshes->DrawBoxMesh();

	// ---------------------------------------------------------------
	// Couch Backrest
	// ---------------------------------------------------------------
	scaleXYZ = glm::vec3(9.00f, 2.50f, 0.80f);
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-3.0f, 1.60f, -6.30f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	m_pShaderManager->setBoolValue("bUseLighting", true);

	// very soft ambient
	m_pShaderManager->setVec3Value("material.ambientColor", glm::vec3(0.40f, 0.40f, 0.40f));
	m_pShaderManager->setFloatValue("material.ambientStrength", 0.20f);

	// absorbs light
	m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(1.5f, 1.5f, 1.5f));

	// remove specular reflection
	m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.0f, 0.0f, 0.0f));
	m_pShaderManager->setFloatValue("material.shininess", 0.0f);

	SetShaderColor(1, 1, 1, 1);

	// turn on texturing + choose the texture
	SetShaderTexture("fabric");

	m_basicMeshes->DrawBoxMesh();

	// ---------------------------------------------------------------
	// Left Armrest
	// ---------------------------------------------------------------
	scaleXYZ = glm::vec3(3.30f, 1.75f, 0.80f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(1.90f, 1.20f, -5.05f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);


	SetShaderColor(1, 1, 1, 1);

	// turn on texturing + choose the texture
	SetShaderTexture("fabric");

	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// ---------------------------------------------------------------
	// Right Armrest
	// ---------------------------------------------------------------
	scaleXYZ = glm::vec3(3.30f, 1.75f, 0.80f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-7.90f, 1.20f, -5.05f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);


	SetShaderColor(1, 1, 1, 1);

	// turn on texturing + choose the texture
	SetShaderTexture("fabric");

	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// ---------------------------------------------------------------
	// Couch Leg (front left)
	// ---------------------------------------------------------------
	scaleXYZ = glm::vec3(1.00f, 1.00f, 0.45f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-7.79f, 0.25f, -3.95f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);


	SetShaderColor(1, 1, 1, 1);

	// turn on texturing + choose the texture
	SetShaderTexture("metal");

	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// ---------------------------------------------------------------
	// Couch Leg (rear left)
	// ---------------------------------------------------------------
	scaleXYZ = glm::vec3(1.00f, 1.00f, 0.45f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-7.79f, 0.25f, -6.19f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);


	SetShaderColor(1, 1, 1, 1);

	// turn on texturing + choose the texture
	SetShaderTexture("metal");

	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// ---------------------------------------------------------------
	// Couch Leg (front right)
	// ---------------------------------------------------------------
	scaleXYZ = glm::vec3(1.00f, 1.00f, 0.45f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(1.79f, 0.25f, -3.95f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);


	SetShaderColor(1, 1, 1, 1);

	// turn on texturing + choose the texture
	SetShaderTexture("metal");

	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// ---------------------------------------------------------------
	// Couch Leg (rear right)
	// ---------------------------------------------------------------
	scaleXYZ = glm::vec3(1.00f, 1.00f, 0.45f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(1.79f, 0.25f, -6.19f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);


	SetShaderColor(1, 1, 1, 1);

	// turn on texturing + choose the texture
	SetShaderTexture("metal");

	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	// -------------------******RUG******----------------------------
	/****************************************************************/

	scaleXYZ = glm::vec3(17.00f, 10.00f, 0.05f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-1.00f, 0.04f, 2.50f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	m_pShaderManager->setBoolValue("bUseLighting", true);

	// very soft ambient
	m_pShaderManager->setVec3Value("material.ambientColor", glm::vec3(0.40f, 0.40f, 0.40f));
	m_pShaderManager->setFloatValue("material.ambientStrength", 0.20f);

	// absorbs light
	m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.60f, 0.60f, 0.60f));

	// remove specular reflection
	m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.0f, 0.0f, 0.0f));
	m_pShaderManager->setFloatValue("material.shininess", 0.0f);


	SetShaderColor(1, 1, 1, 1);

	// turn on texturing + choose the texture
	SetShaderTexture("rug");

	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	// -----------------******COFFEE TABLE******---------------------
	/****************************************************************/

	// ---------------------------------------------------------------
	// Table Top
	// ---------------------------------------------------------------

	scaleXYZ = glm::vec3(7.25f, 3.00f, 0.15f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-3.00f, 1.00f, 1.50f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	m_pShaderManager->setBoolValue("bUseLighting", true);

	SetShaderColor(1, 1, 1, 1);

	// turn on texturing + choose the texture
	SetShaderTexture("glass");

	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// ---------------------------------------------------------------
	// Table Base
	// ---------------------------------------------------------------

	scaleXYZ = glm::vec3(7.25f, 3.00f, 0.15f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-3.00f, 0.20f, 1.50f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	m_pShaderManager->setBoolValue("bUseLighting", true);

	SetShaderColor(1, 1, 1, 1);

	// turn on texturing + choose the texture
	SetShaderTexture("glass");

	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// ---------------------------------------------------------------
	// Table Supports
	// ---------------------------------------------------------------

	// Right front ---------------------------------------------------

	scaleXYZ = glm::vec3(0.15f, 0.65f, 0.15f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(.50f, 0.60f, 2.90f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	m_pShaderManager->setBoolValue("bUseLighting", true);

	SetShaderColor(1, 1, 1, 1);

	// turn on texturing + choose the texture
	SetShaderTexture("metal");

	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// Right rear ---------------------------------------------------
	
	scaleXYZ = glm::vec3(0.15f, 0.65f, 0.15f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(.50f, 0.60f, 0.10f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	m_pShaderManager->setBoolValue("bUseLighting", true);

	SetShaderColor(1, 1, 1, 1);

	// turn on texturing + choose the texture
	SetShaderTexture("metal");

	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// Left front ---------------------------------------------------

	scaleXYZ = glm::vec3(0.15f, 0.65f, 0.15f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-6.50f, 0.60f, 2.90f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	m_pShaderManager->setBoolValue("bUseLighting", true);

	SetShaderColor(1, 1, 1, 1);

	// turn on texturing + choose the texture
	SetShaderTexture("metal");

	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// Left rear ---------------------------------------------------

	scaleXYZ = glm::vec3(0.15f, 0.65f, 0.15f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-6.50f, 0.60f, 0.10f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	m_pShaderManager->setBoolValue("bUseLighting", true);

	SetShaderColor(1, 1, 1, 1);

	// turn on texturing + choose the texture
	SetShaderTexture("metal");

	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();


	/****************************************************************/
	// -----------------******TV STAND******-------------------------
	/****************************************************************/

	// Table top -----------------------------------------------------
	scaleXYZ = glm::vec3(7.25f, 3.00f, 1.00f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	positionXYZ = glm::vec3(8.49f, 1.00f, 2.50f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	m_pShaderManager->setBoolValue("bUseLighting", true);

	SetShaderColor(1, 1, 1, 1);

	// turn on texturing + choose the texture
	SetShaderTexture("whiteWood");

	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// Base -----------------------------------------------------------
	scaleXYZ = glm::vec3(7.00f, 2.50f, .75f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	positionXYZ = glm::vec3(8.74f, 0.40f, 2.50f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	m_pShaderManager->setBoolValue("bUseLighting", true);

	SetShaderColor(1, 1, 1, 1);

	// turn on texturing + choose the texture
	SetShaderTexture("metal");

	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	// -----------------******TELEVISION******-----------------------
	/****************************************************************/

	// TV body -------------------------------------------------------
	scaleXYZ = glm::vec3(6.00f, 3.00f, 0.20f);

	XrotationDegrees = 180.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(9.89f, 5.00f, 2.50f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	m_pShaderManager->setBoolValue("bUseLighting", true);

	SetShaderColor(0, 0, 0, 1);

	m_basicMeshes->DrawBoxMesh();

	// TV screen -------------------------------------------------------

	scaleXYZ = glm::vec3(5.75f, 2.80f, 0.01f);

	XrotationDegrees = 180.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(9.79f, 5.00f, 2.50f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	m_pShaderManager->setBoolValue("bUseLighting", true);

	SetShaderColor(1, 1, 1, 1);

	// turn on texturing + choose the texture
	SetShaderTexture("TV");

	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

}
