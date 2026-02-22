///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

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
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
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

	// matrix math for calculating the final model matrix
	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		// pass the model matrix into the shader
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
		// pass the color values into the shader
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();       // cube pedestal + tall box + small left block
	m_basicMeshes->LoadCylinderMesh();  // cylinder sitting on pedestal
	m_basicMeshes->LoadSphereMesh();    // sphere in front

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

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	// ------------------------------------------------------------
	// FLOOR PLANE
	// ------------------------------------------------------------
	
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

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
	// set the color values into the shader
	SetShaderColor(1.0f, 0.70f, 0.80f, 1.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// ------------------------------------------------------------
	// BACK WALL PLANE
	// ------------------------------------------------------------

	/****************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f; // Rotate plane 90 degrees around X to stand it upright.
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 9.0f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(1.0f, 0.80f, 0.92f, 1.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// ------------------------------------------------------------
	// SMALL LEFT RECTANGULAR BLOCK 
	// ------------------------------------------------------------
	scaleXYZ = glm::vec3(4.0f, 1.0f, 2.0f); 

	XrotationDegrees = 0.0f; 
	YrotationDegrees = 0.0f; 
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-7.0f, 0.5f, 3.5f);  

	SetTransformations(
		scaleXYZ, 
		XrotationDegrees, 
		YrotationDegrees, 
		ZrotationDegrees, 
		positionXYZ);

	SetShaderColor(0.98f, 0.90f, 0.97f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// ------------------------------------------------------------
	// CENTER PEDESTAL CUBE
	// ------------------------------------------------------------

	scaleXYZ = glm::vec3(6.0f, 3.5f, 6.0f); 

	XrotationDegrees = 0.0f; 
	YrotationDegrees = 50.0f; 
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-2.5f, 1.75f, 0.5f); 

	SetTransformations(
		scaleXYZ, 
		XrotationDegrees, 
		YrotationDegrees, 
		ZrotationDegrees, 
		positionXYZ);

	SetShaderColor(0.98f, 0.88f, 0.96f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// ------------------------------------------------------------
	// CYLINDER ON TOP OF CENTER PEDESTAL
	// ------------------------------------------------------------

	scaleXYZ = glm::vec3(1.0f, 3.5f, 1.0f); 

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-0.25f, 3.5f, 0.5f);

	SetTransformations(
		scaleXYZ, 
		XrotationDegrees, 
		YrotationDegrees, 
		ZrotationDegrees, 
		positionXYZ);

	SetShaderColor(0.72f, 0.55f, 0.50f, 1.0f);   

	m_basicMeshes->DrawCylinderMesh();

	// ------------------------------------------------------------
	// TALL RIGHT BOX
	// ------------------------------------------------------------

	scaleXYZ = glm::vec3(6.0f, 8.5f, 2.0f);

	XrotationDegrees = 0.0f; 
	YrotationDegrees = 25.0f; 
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(6.5f, 8.5f / 2.0f, -1.0f); 

	SetTransformations(
		scaleXYZ, 
		XrotationDegrees, 
		YrotationDegrees, 
		ZrotationDegrees, 
		positionXYZ);

	SetShaderColor(0.97f, 0.86f, 0.96f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// ------------------------------------------------------------
	// SPHERE 
	// ------------------------------------------------------------

	scaleXYZ = glm::vec3(1.8f, 1.8f, 1.8f);  // uniform scale makes it a sphere

	XrotationDegrees = 0.0f; 
	YrotationDegrees = 0.0f; 
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(2.0f, 1.8f, 1.8f);  

	SetTransformations(
		scaleXYZ, 
		XrotationDegrees, 
		YrotationDegrees, 
		ZrotationDegrees, 
		positionXYZ);

	SetShaderColor(0.95f, 0.45f, 0.70f, 1.0f);  

	m_basicMeshes->DrawSphereMesh();

}