#include "A1.hpp"
#include "cs488-framework/GlErrorCheck.hpp"

#include <iostream>

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;
using namespace std;

static const size_t DIM = 16;
static const size_t COLOURS = 8;
static const size_t VERTICES_IN_CUBE = 36;
static const int MAX_HEIGHT = 8;
static const float MAX_SCALE = 1.5f;
static const float MIN_SCALE = 0.7f;

//----------------------------------------------------------------------------------------
// Constructor
A1::A1()
	: current_col(0),
	  active_col(0),
	  active_row(0),
	  grid (DIM),
	  is_dragging(false),
	  prev_x_pos(-1),
	  scale_ratio(1.0f)
{
	colours = new float*[COLOURS];
	for (int i = 0; i < COLOURS; i++) {
		colours[i] = new float[3];
	}
}

//----------------------------------------------------------------------------------------
// Destructor
A1::~A1()
{}

//----------------------------------------------------------------------------------------
/*
 * Called once, at program start.
 */
void A1::init()
{
	// Set the background colour.
	glClearColor( 0.3, 0.5, 0.7, 1.0 );

	// Build the shader
	m_shader.generateProgramObject();
	m_shader.attachVertexShader(
		getAssetFilePath( "VertexShader.vs" ).c_str() );
	m_shader.attachFragmentShader(
		getAssetFilePath( "FragmentShader.fs" ).c_str() );
	m_shader.link();

	// Set up the uniforms
	P_uni = m_shader.getUniformLocation( "P" );
	V_uni = m_shader.getUniformLocation( "V" );
	M_uni = m_shader.getUniformLocation( "M" );
	col_uni = m_shader.getUniformLocation( "colour" );

	reset();
	initGrid();
	
	
	// Set up initial view and projection matrices (need to do this here,
	// since it depends on the GLFW window being set up correctly).
	view = glm::lookAt( 
		glm::vec3( 0.0f, float(DIM)*2.0*M_SQRT1_2, float(DIM)*2.0*M_SQRT1_2 ),
		glm::vec3( 0.0f, 0.0f, 0.0f ),
		glm::vec3( 0.0f, 1.0f, 0.0f ) );
	proj = glm::perspective( 
		glm::radians( 45.0f ),
		float( m_framebufferWidth ) / float( m_framebufferHeight ),
		1.0f, 1000.0f );
}

void A1::prepareActiveCell()
{
	// Vertices for active cell indicator
	float verts[] = {
		0.5f, 0.0f, 0.25f,
		0.75f, 0.0f, 0.5f,
		0.5f, -1.0f, 0.5f,

		0.5f, 0.0f, 0.25f,
		0.25f, 0.0f, 0.5f,
		0.5f, -1.0f, 0.5f,

		0.25f, 0.0f, 0.5f,
		0.5f, 0.0f, 0.75f,
		0.5f, -1.0f, 0.5f,

		0.75f, 0.0f, 0.5f,
		0.5f, 0.0f, 0.75f,
		0.5f, -1.0f, 0.5f,

		0.5f, 0.0f, 0.25f,
		0.75f, 0.0f, 0.5f,
		0.5f, 1.0f, 0.5f,

		0.5f, 0.0f, 0.25f,
		0.25f, 0.0f, 0.5f,
		0.5f, 1.0f, 0.5f,

		0.25f, 0.0f, 0.5f,
		0.5f, 0.0f, 0.75f,
		0.5f, 1.0f, 0.5f,

		0.75f, 0.0f, 0.5f,
		0.5f, 0.0f, 0.75f,
		0.5f, 1.0f, 0.5f,
	};

	// Add x, y, z displacements to vertices
	int i = 0;
	for (int idx = 0; idx < 24; idx++) {
		verts[i] += active_col;
		verts[i + 1] += grid.getHeight(active_col, active_row) + 2;
		verts[i + 2] += active_row;
		i += 3;
	}

	glGenVertexArrays(1, &m_active_cell_vao);
	glBindVertexArray(m_active_cell_vao);
	
	glGenBuffers(1, &m_active_cell_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_active_cell_vbo);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(verts),
		verts,
		GL_STATIC_DRAW); 

	GLint posAttrib = m_shader.getAttribLocation("position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
}

void A1::prepareCubeAt(float xPos, float yPos, float zPos)
{
	// Vertices for a cube
	float verts[] = {
		0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 1.0f,
	};

	// Add x, y, z displacements to vertices
	int i = 0;
	for (int idx = 0; idx < VERTICES_IN_CUBE; idx++) {
		verts[i] = verts[i] + xPos;
		verts[i+1] = verts[i+1] + yPos;
		verts[i+2] = verts[i+2] + zPos;
		i += 3;
	}

	glGenVertexArrays(1, &m_cube_vao);
	glBindVertexArray(m_cube_vao);

	glGenBuffers(1, &m_cube_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_cube_vbo);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(verts),
		verts,
		GL_STATIC_DRAW);
	
	GLint posAttrib = m_shader.getAttribLocation("position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
}

void A1::initGrid()
{
	size_t sz = 3 * 2 * 2 * (DIM+3);

	float *verts = new float[ sz ];
	size_t ct = 0;
	for( int idx = 0; idx < DIM+3; ++idx ) {
		verts[ ct ] = -1;
		verts[ ct+1 ] = 0;
		verts[ ct+2 ] = idx-1;
		verts[ ct+3 ] = DIM+1;
		verts[ ct+4 ] = 0;
		verts[ ct+5 ] = idx-1;
		ct += 6;

		verts[ ct ] = idx-1;
		verts[ ct+1 ] = 0;
		verts[ ct+2 ] = -1;
		verts[ ct+3 ] = idx-1;
		verts[ ct+4 ] = 0;
		verts[ ct+5 ] = DIM+1;
		ct += 6;
	}

	// Create the vertex array to record buffer assignments.
	glGenVertexArrays( 1, &m_grid_vao );
	glBindVertexArray( m_grid_vao );

	// Create the cube vertex buffer
	glGenBuffers( 1, &m_grid_vbo );
	glBindBuffer( GL_ARRAY_BUFFER, m_grid_vbo );
	glBufferData( GL_ARRAY_BUFFER, sz * sizeof(float),
		verts, GL_STATIC_DRAW );

	// Specify the means of extracting the position values properly.
	GLint posAttrib = m_shader.getAttribLocation( "position" );
	glEnableVertexAttribArray( posAttrib );
	glVertexAttribPointer( posAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr );

	// Reset state to prevent rogue code from messing with *my* 
	// stuff!
	glBindVertexArray( 0 );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

	// OpenGL has the buffer now, there's no need for us to keep a copy.
	delete [] verts;

	CHECK_GL_ERRORS;
}

void A1::reset()
{
	// Resets grid, active cell and colour
	grid.reset();
	active_col = 0;
	active_row = 0;
	current_col = 0;

	// Green
	colours[0][0] = 0.0f;
	colours[0][1] = 0.392f;
	colours[0][2] = 0.0f;

	// Lavender
	colours[1][0] = 0.902f;
	colours[1][1] = 0.902f;
	colours[1][2] = 0.98f;

	// Light yellow
	colours[2][0] = 1.0f;
	colours[2][1] = 0.98f;
	colours[2][2] = 0.804f;

	// Blue
	colours[3][0] = 0.255f;
	colours[3][1] = 0.412f;
	colours[3][2] = 0.882f;

	// Brown
	colours[4][0] = 0.545f;
	colours[4][1] = 0.271;
	colours[4][2] = 0.075f;

	// Pink
	colours[5][0] = 1.0f;
	colours[5][1] = 0.753f;
	colours[5][2] = 0.796f;

	// Orange
	colours[6][0] = 1.0f;
	colours[6][1] = 0.647f;
	colours[6][2] = 0.0f;
	
	// White
	colours[7][0] = 1.0f;
	colours[7][1] = 1.0f;
	colours[7][2] = 1.0f;

	// Resets view
	world = mat4();
	world = glm::translate(world, vec3(-float(DIM) / 2.0f, 0, -float(DIM) / 2.0f));
	scale_ratio = 1.0f;
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void A1::appLogic()
{
	// Place per frame, application logic here ...
	prepareActiveCell();
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void A1::guiLogic()
{
	// We already know there's only going to be one window, so for 
	// simplicity we'll store button states in static local variables.
	// If there was ever a possibility of having multiple instances of
	// A1 running simultaneously, this would break; you'd want to make
	// this into instance fields of A1.
	static bool showTestWindow(false);
	static bool showDebugWindow(true);

	ImGuiWindowFlags windowFlags(ImGuiWindowFlags_AlwaysAutoResize);
	float opacity(0.5f);

	ImGui::Begin("Debug Window", &showDebugWindow, ImVec2(100,100), opacity, windowFlags);
		if( ImGui::Button( "Quit Application" ) ) {
			glfwSetWindowShouldClose(m_window, GL_TRUE);
		}

		// Eventually you'll create multiple colour widgets with
		// radio buttons.  If you use PushID/PopID to give them all
		// unique IDs, then ImGui will be able to keep them separate.
		// This is unnecessary with a single colour selector and
		// radio button, but I'm leaving it in as an example.

		// Prefixing a widget name with "##" keeps it from being
		// displayed.
		for (int i = 0; i < 8; i++) {
			ImGui::PushID(i);
			ImGui::ColorEdit3("##Colour", colours[i]);
			ImGui::SameLine();
			if (ImGui::RadioButton("##Col", &current_col, i)) {
				// Select this colour.
				current_col = i;
				grid.setColour(active_col, active_row, current_col);
			}
			ImGui::PopID();
		}

		if (ImGui::Button("Reset")) {
			reset();
		}
/*
		// For convenience, you can uncomment this to show ImGui's massive
		// demonstration window right in your application.  Very handy for
		// browsing around to get the widget you want.  Then look in 
		// shared/imgui/imgui_demo.cpp to see how it's done.
		if( ImGui::Button( "Test Window" ) ) {
			showTestWindow = !showTestWindow;
		}
*/

		ImGui::Text( "Framerate: %.1f FPS", ImGui::GetIO().Framerate );

	ImGui::End();

	if( showTestWindow ) {
		ImGui::ShowTestWindow( &showTestWindow );
	}
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void A1::draw()
{
	m_shader.enable();
		glEnable( GL_DEPTH_TEST );

		glUniformMatrix4fv( P_uni, 1, GL_FALSE, value_ptr( proj ) );
		glUniformMatrix4fv( V_uni, 1, GL_FALSE, value_ptr( view ) );
		glUniformMatrix4fv( M_uni, 1, GL_FALSE, value_ptr( world ) );

		// Just draw the grid for now.
		glBindVertexArray( m_grid_vao );
		glUniform3f( col_uni, 1, 1, 1 );
		glDrawArrays( GL_LINES, 0, (3+DIM)*4 );

		// Draw the cubes
		for (int i = 0; i < DIM; i++) {
			for (int j = 0; j < DIM; j++) {
				int colour_index = grid.getColour(i, j);
				glUniform3f(
					col_uni, 
					colours[colour_index][0], 
					colours[colour_index][1], 
					colours[colour_index][2]);
				for (int k = 0; k < grid.getHeight(i, j); k++) {
					prepareCubeAt(i, k, j);
					glBindVertexArray(m_cube_vao);
					glDrawArrays(GL_TRIANGLES, 0, VERTICES_IN_CUBE);
				}
			}
		}

		// Highlight the active square.
		glBindVertexArray(m_active_cell_vao);
		glUniform3f(col_uni, 0.878f, 1.0f, 1.0f);
		glDrawArrays(GL_TRIANGLES, 0, 24);
	m_shader.disable();

	// Restore defaults
	glBindVertexArray( 0 );

	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
/*
 * Called once, after program is signaled to terminate.
 */
void A1::cleanup()
{}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles cursor entering the window area events.
 */
bool A1::cursorEnterWindowEvent (int entered)
{
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse cursor movement events.
 */
bool A1::mouseMoveEvent(double xPos, double yPos) 
{
	bool eventHandled(false);

	if (!ImGui::IsMouseHoveringAnyWindow()) {
		// Put some code here to handle rotations.  Probably need to
		// check whether we're *dragging*, not just moving the mouse.
		// Probably need some instance variables to track the current
		// rotation amount, and maybe the previous X position (so 
		// that you can rotate relative to the *change* in X.
		if (is_dragging) {
			double deltaX = xPos - prev_x_pos;
			world = glm::translate(world, vec3(float(DIM) / 2.0f, 0, float(DIM) / 2.0f));
			world = glm::rotate(world, glm::radians(float(deltaX/3)), vec3(0, 1, 0));
			world = glm::translate(world, vec3(-float(DIM) / 2.0f, 0, -float(DIM) / 2.0f));
		}
		prev_x_pos = xPos;
		eventHandled = true;
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse button events.
 */
bool A1::mouseButtonInputEvent(int button, int actions, int mods)
{
	bool eventHandled(false);

	if (!ImGui::IsMouseHoveringAnyWindow()) {
		// The user clicked in the window.  If it's the left
		// mouse button, initiate a rotation.
		is_dragging = button == GLFW_MOUSE_BUTTON_LEFT && actions == GLFW_PRESS;
		eventHandled = true;
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll wheel events.
 */
bool A1::mouseScrollEvent(double xOffSet, double yOffSet)
{
	bool eventHandled(false);

	// Zoom in or out.
	world = glm::translate(world, vec3(float(DIM) / 2.0f, 0, float(DIM) / 2.0f));
	world = glm::scale(world, vec3(1 / scale_ratio, 1 / scale_ratio, 1 / scale_ratio));
	scale_ratio += float(yOffSet / 40);
	// Limit the scale ratio within reasonable bounds
	scale_ratio = glm::min(scale_ratio, MAX_SCALE);
	scale_ratio = glm::max(scale_ratio, MIN_SCALE);
	world = glm::scale(world, vec3(scale_ratio, scale_ratio, scale_ratio));
	world = glm::translate(world, vec3(-float(DIM) / 2.0f, 0, -float(DIM) / 2.0f));
	eventHandled = true;

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles window resize events.
 */
bool A1::windowResizeEvent(int width, int height)
{
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles key input events.
 */
bool A1::keyInputEvent(int key, int action, int mods)
{
	bool eventHandled(false);

	// Fill in with event handling code...
	if( action == GLFW_PRESS ) {
		// Respond to some key events.
		switch(key) {
			case GLFW_KEY_SPACE:
				// SPACE key
				if (grid.getHeight(active_col, active_row) < MAX_HEIGHT) {
					// Change colour to active colour on empty cells
					if (grid.getHeight(active_col, active_row) == 0) {
						grid.setColour(active_col, active_row, current_col);
					}
					grid.setHeight(active_col, active_row, grid.getHeight(active_col, active_row) + 1);
				}
				break;
			case GLFW_KEY_BACKSPACE:
				// BACKSPACE Key
				if (grid.getHeight(active_col, active_row) > 0) {
					grid.setHeight(active_col, active_row, grid.getHeight(active_col, active_row) - 1);
				}
				break;
			case GLFW_KEY_RIGHT:
				// RIGHT key
				if (active_col < DIM - 1) {
					active_col ++;
					if (mods == GLFW_MOD_SHIFT) {
						grid.setHeight(active_col, active_row, grid.getHeight(active_col - 1, active_row));
						grid.setColour(active_col, active_row, grid.getColour(active_col - 1, active_row));
					}
				}
				break;
			case GLFW_KEY_LEFT:
				// LEFT key
				if (active_col > 0) {
					active_col --;
					if (mods == GLFW_MOD_SHIFT) {
						grid.setHeight(active_col, active_row, grid.getHeight(active_col + 1, active_row));
						grid.setColour(active_col, active_row, grid.getColour(active_col + 1, active_row));
					}
				}
				break;
			case GLFW_KEY_DOWN:
				// DOWN key
				if (active_row < DIM - 1) {
					active_row++;
					if (mods == GLFW_MOD_SHIFT) {
						grid.setHeight(active_col, active_row, grid.getHeight(active_col, active_row - 1));
						grid.setColour(active_col, active_row, grid.getColour(active_col, active_row - 1));
					}
				}
				break;
			case GLFW_KEY_UP:
				// UP key
				if (active_row > 0) {
					active_row--;
					if (mods == GLFW_MOD_SHIFT) {
						grid.setHeight(active_col, active_row, grid.getHeight(active_col, active_row + 1));
						grid.setColour(active_col, active_row, grid.getColour(active_col, active_row + 1));
					}
				}
				break;
			case GLFW_KEY_R:
				// R key
				reset();
				break;
			case GLFW_KEY_Q:
				// Q key
				glfwSetWindowShouldClose(m_window, GL_TRUE);
				break;
		}
	}

	return eventHandled;
}
