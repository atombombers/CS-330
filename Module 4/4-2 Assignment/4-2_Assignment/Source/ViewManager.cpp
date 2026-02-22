///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

#include <iostream>

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;

	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// camera object used for viewing and interacting with the 3D scene
	Camera* g_pCamera = nullptr;

	// Mouse state for look/orbit input processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// Per-frame timing
	float gDeltaTime = 0.0f;
	float gLastFrame = 0.0f;

	// Projection toggle (left here for future extension)
	bool bOrthographicProjection = false;

	// ------------------------------------------------------
	// Movement + orbit configuration
	// ------------------------------------------------------

	// Scroll wheel adjusts how fast the camera moves (and how fast orbit rotates).
	// Capped so it never becomes unusably slow or ridiculously fast.
	float gMoveSpeed = 6.0f;      
	const float gMoveSpeedMin = 1.0f;
	const float gMoveSpeedMax = 20.0f;

	// Mouse sensitivity for camera look/orbit
	float gMouseSensitivity = 0.1f;

	// Orbit behavior:
	//   - Hold left mouse button and drag to orbit around a target point.
	bool gIsOrbiting = false;

	// Orbit target (the point the camera orbits around).
	glm::vec3 gOrbitTarget = glm::vec3(0.0f, 0.0f, 0.0f);

	// Orbit angles (degrees) and distance from target
	float gOrbitYaw = -90.0f;
	float gOrbitPitch = 0.0f;
	float gOrbitRadius = 10.0f;

	// Helper: world up axis
	const glm::vec3 gWorldUp = glm::vec3(0.0f, 1.0f, 0.0f);

	// Utility: safely normalize (prevents NaN when length is ~0)
	glm::vec3 SafeNormalize(const glm::vec3& v)
	{
		const float len = glm::length(v);
		if (len < 0.00001f) return glm::vec3(0.0f, 0.0f, 0.0f);
		return v / len;
	}

	// Rebuild camera Up vector using a stable world-up reference.
	// This avoids camera roll and keeps movement predictable.
	void RebuildCameraUpFromWorldUp()
	{
		if (g_pCamera == nullptr) return;

		glm::vec3 front = SafeNormalize(g_pCamera->Front);
		glm::vec3 right = SafeNormalize(glm::cross(front, gWorldUp));
		glm::vec3 up = SafeNormalize(glm::cross(right, front));

		// Update camera up so GetViewMatrix stays consistent
		g_pCamera->Up = up;
	}

	// Compute orbit camera position & orientation from orbit yaw/pitch/radius.
	void ApplyOrbitToCamera()
	{
		if (g_pCamera == nullptr) return;

		// Cap pitch so we never flip upside-down
		if (gOrbitPitch < -89.0f) gOrbitPitch = -89.0f;
		if (gOrbitPitch > 89.0f) gOrbitPitch = 89.0f;

		// Convert degrees to radians for trig
		const float yawRad = glm::radians(gOrbitYaw);
		const float pitchRad = glm::radians(gOrbitPitch);

		// Spherical coordinates -> Cartesian offset from target
		glm::vec3 offset;
		offset.x = gOrbitRadius * cos(pitchRad) * cos(yawRad);
		offset.y = gOrbitRadius * sin(pitchRad);
		offset.z = gOrbitRadius * cos(pitchRad) * sin(yawRad);

		// Set camera position around the target
		g_pCamera->Position = gOrbitTarget + offset;

		// Make the camera look at the target (front points from camera to target)
		g_pCamera->Front = SafeNormalize(gOrbitTarget - g_pCamera->Position);

		// Rebuild Up so the view matrix stays stable and no rolling occurs
		RebuildCameraUpFromWorldUp();
	}
}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(ShaderManager* pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;

	// Create the camera used for viewing/interacting with the scene
	g_pCamera = new Camera();

	// Default camera view parameters (starting position and orientation)
	g_pCamera->Position = glm::vec3(0.5f, 5.5f, 10.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80.0f;

	// Initialize orbit values based on the starting camera position:
	// radius is distance from target, yaw/pitch start at reasonable defaults.
	gOrbitRadius = glm::length(g_pCamera->Position - gOrbitTarget);
	gOrbitYaw = -90.0f;
	gOrbitPitch = 0.0f;

	RebuildCameraUpFromWorldUp();
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	m_pShaderManager = NULL;
	m_pWindow = NULL;

	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, windowTitle, NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// Callback: mouse movement (look + orbit)
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);

	// Callback: mouse scroll (adjust movement/orbit speed)
	glfwSetScrollCallback(window, &ViewManager::Mouse_Scroll_Callback);

	// Callback: mouse button press (toggle orbit)
	glfwSetMouseButtonCallback(window, &ViewManager::Mouse_Button_Callback);

	// enable blending for supporting transparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;
	return window;
}

/***********************************************************
 *  Mouse_Button_Callback()
 *
 *  Orbiting mode is enabled while LEFT mouse button is held.
 *  This lets you:
 *    - look around normally when not holding left mouse
 *    - orbit around the target when holding left mouse
 ***********************************************************/
void ViewManager::Mouse_Button_Callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS)
		{
			gIsOrbiting = true;

			// Reset "first mouse" so the first orbit drag doesn't jump
			gFirstMouse = true;

			// Update orbit radius based on current camera position (keeps orbit consistent)
			if (g_pCamera != nullptr)
			{
				gOrbitRadius = glm::length(g_pCamera->Position - gOrbitTarget);

				// Try to initialize orbit yaw/pitch from current camera direction
				glm::vec3 dirFromTarget = SafeNormalize(g_pCamera->Position - gOrbitTarget);
				// Avoid NaN if camera is exactly at target
				if (glm::length(dirFromTarget) > 0.00001f)
				{
					// Convert direction vector back into yaw/pitch
					gOrbitPitch = glm::degrees(asin(dirFromTarget.y));
					gOrbitYaw = glm::degrees(atan2(dirFromTarget.z, dirFromTarget.x));
				}
			}
		}
		else if (action == GLFW_RELEASE)
		{
			gIsOrbiting = false;
			gFirstMouse = true;
		}
	}
}

/***********************************************************
 *  Mouse_Scroll_Callback()
 *
 *  Mouse scroll should adjust the speed of movement OR orbit speed.
 *  Here we adjust gMoveSpeed (which is capped). This affects:
 *    - WASD + QE translation speed
 *    - Orbit rotation responsiveness (indirectly, by scaling sensitivity)
 ***********************************************************/
void ViewManager::Mouse_Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset)
{
	// Scroll up -> faster, scroll down -> slower
	gMoveSpeed += static_cast<float>(yOffset);

	// Cap so it stays usable
	if (gMoveSpeed < gMoveSpeedMin) gMoveSpeed = gMoveSpeedMin;
	if (gMoveSpeed > gMoveSpeedMax) gMoveSpeed = gMoveSpeedMax;
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 * 
 *  - Normal mode: mouse changes camera orientation (look around)
 *  - Orbit mode (hold left mouse): mouse orbits camera around target
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	// Record first movement to prevent jump when starting to move mouse
	if (gFirstMouse)
	{
		gLastX = static_cast<float>(xMousePos);
		gLastY = static_cast<float>(yMousePos);
		gFirstMouse = false;
	}

	// Calculate mouse offsets (delta movement)
	float xOffset = static_cast<float>(xMousePos) - gLastX;
	float yOffset = gLastY - static_cast<float>(yMousePos); // reversed since y-coordinates go bottom->top

	// Update last positions
	gLastX = static_cast<float>(xMousePos);
	gLastY = static_cast<float>(yMousePos);

	// Apply sensitivity (and scale slightly by speed so orbit feels consistent)
	const float scaledSensitivity = gMouseSensitivity * (gMoveSpeed / 6.0f);
	xOffset *= scaledSensitivity;
	yOffset *= scaledSensitivity;

	if (g_pCamera == nullptr)
		return;

	if (gIsOrbiting)
	{
		// ORBIT: adjust yaw/pitch around the target, then recompute camera position
		gOrbitYaw += xOffset;
		gOrbitPitch += yOffset;

		ApplyOrbitToCamera();
	}
	else
	{

		// Convert current Front vector into yaw/pitch, add offsets, and rebuild Front.
		glm::vec3 f = SafeNormalize(g_pCamera->Front);

		// If front is degenerate, reset to a safe direction
		if (glm::length(f) < 0.00001f)
			f = glm::vec3(0.0f, 0.0f, -1.0f);

		float yaw = glm::degrees(atan2(f.z, f.x));
		float pitch = glm::degrees(asin(f.y));

		yaw += xOffset;
		pitch += yOffset;

		// Cap pitch so the camera never flips
		if (pitch < -89.0f) pitch = -89.0f;
		if (pitch > 89.0f) pitch = 89.0f;

		// Convert yaw/pitch back into a normalized front vector
		const float yawRad = glm::radians(yaw);
		const float pitchRad = glm::radians(pitch);

		glm::vec3 front;
		front.x = cos(pitchRad) * cos(yawRad);
		front.y = sin(pitchRad);
		front.z = cos(pitchRad) * sin(yawRad);

		g_pCamera->Front = SafeNormalize(front);
		RebuildCameraUpFromWorldUp();
	}
}

/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// Close the window if ESC is pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	if (g_pCamera == nullptr)
		return;

	// Compute the translation velocity using delta time
	const float velocity = gMoveSpeed * gDeltaTime;

	// Build a right vector from Front and WorldUp for consistent strafing
	glm::vec3 front = SafeNormalize(g_pCamera->Front);
	glm::vec3 right = SafeNormalize(glm::cross(front, gWorldUp));

	// ------------------------------------------------------
	// WASD movement (pan/zoom via translation)
	// ------------------------------------------------------

	// W = move forward (zoom in by moving toward what you're looking at)
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
	{
		g_pCamera->Position += front * velocity;
	}
	// S = move backward (zoom out by moving away from what you're looking at)
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
	{
		g_pCamera->Position -= front * velocity;
	}
	// A = move left (pan left)
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
	{
		g_pCamera->Position -= right * velocity;
	}
	// D = move right (pan right)
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
	{
		g_pCamera->Position += right * velocity;
	}

	// ------------------------------------------------------
	// QE vertical movement
	// ------------------------------------------------------

	// Q = move down
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
	{
		g_pCamera->Position -= gWorldUp * velocity;
	}
	// E = move up
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
	{
		g_pCamera->Position += gWorldUp * velocity;
	}

}

/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// Per-frame timing (delta time)
	float currentFrame = static_cast<float>(glfwGetTime());
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// Process keyboard input for movement/panning/zooming
	ProcessKeyboardEvents();

	// Get the current view matrix from the camera
	view = g_pCamera->GetViewMatrix();

	// Define the current projection matrix (perspective by default)
	projection = glm::perspective(
		glm::radians(g_pCamera->Zoom),
		(GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT,
		0.1f,
		100.0f
	);

	// Send matrices and view position to the shader
	if (m_pShaderManager != NULL)
	{
		m_pShaderManager->setMat4Value(g_ViewName, view);
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
	}
}
