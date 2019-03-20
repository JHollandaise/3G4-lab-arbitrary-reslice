#include <wx/file.h>
#include <GL/glu.h>
#include "wx_icon.xpm"
#include "vis.h"
#include "linear_interp.h"

using namespace std;

// Static pointer to the class containing all the windows
MyFrame *frame;         

// Constants for GL surface materials
const GLfloat mat_diffuse[] = { 0.0, 0.0, 0.0, 1.0 };
const GLfloat mat_no_specular[] = { 0.0, 0.0, 0.0, 0.0 };
const GLfloat mat_no_shininess[] = { 0.0 };
const GLfloat mat_specular[] = { 0.5, 0.5, 0.5, 1.0 };
const GLfloat mat_shininess[] = { 50.0 };

// Constants for GL lights
const GLfloat top_right[] = { 1.0, 1.0, 1.0, 0.0 };
const GLfloat straight_on[] = { 0.0, 0.0, 1.0, 0.0 };
const GLfloat no_ambient[] = { 0.0, 0.0, 0.0, 1.0 }; 
const GLfloat dim_diffuse[] = { 0.5, 0.5, 0.5, 1.0 };
const GLfloat bright_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
const GLfloat med_diffuse[] = { 0.75, 0.75, 0.75, 1.0 };
const GLfloat full_specular[] = { 0.5, 0.5, 0.5, 1.0 };
const GLfloat no_specular[] = { 0.0, 0.0, 0.0, 1.0 };

// MyGLCanvas ////////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(MyGLCanvas, wxGLCanvas)
  EVT_SIZE(MyGLCanvas::OnSize)
  EVT_PAINT(MyGLCanvas::OnPaint)
  EVT_MOUSE_EVENTS(MyGLCanvas::OnMouse)
  EVT_KEY_DOWN(MyGLCanvas::OnKey)
END_EVENT_TABLE()
  
int wxglcanvas_attrib_list[5] = {WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 16, 0};

MyGLCanvas::MyGLCanvas(wxWindow *parent, wxWindowID id, canvas_t type, const wxPoint& pos, const wxSize& size, long style, const wxString& name, const wxPalette& palette):
  wxGLCanvas(parent, id, wxglcanvas_attrib_list, pos, size, style, name, palette)
  // Constructor - initialises variables
{
  context = new wxGLContext(this);
  canvas_type = type;
  configured = false;
  wireframe = false;
}

void MyGLCanvas::ConfigureGL()
  // Configures the OpenGL drawing context
{
  int i, w, h;
  GetClientSize(&w, &h);

  // Set up the modelview matrix and the viewport - same for all canvases
  glDrawBuffer(GL_BACK);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glViewport(0, 0, (GLint) w, (GLint) h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  if (canvas_type == CANVAS_TL) { // the 3D view window

    if (!glIsTexture(frame->data_texture)) { 

      // This is the first time the GL context has been configured - set up the variables that do not depend on the window size or the view angle
      glLightfv(GL_LIGHT0, GL_AMBIENT, no_ambient);
      glLightfv(GL_LIGHT0, GL_DIFFUSE, bright_diffuse);
      glLightfv(GL_LIGHT0, GL_SPECULAR, full_specular);
      glLightfv(GL_LIGHT0, GL_POSITION, straight_on);
      glLightfv(GL_LIGHT1, GL_AMBIENT, no_ambient);
      glLightfv(GL_LIGHT1, GL_DIFFUSE, med_diffuse);
      glLightfv(GL_LIGHT1, GL_SPECULAR, no_specular);
      glLightfv(GL_LIGHT1, GL_POSITION, top_right);
      glLightfv(GL_LIGHT2, GL_AMBIENT, no_ambient);
      glLightfv(GL_LIGHT2, GL_DIFFUSE, dim_diffuse);
      glLightfv(GL_LIGHT2, GL_SPECULAR, no_specular);
      glLightfv(GL_LIGHT2, GL_POSITION, straight_on);
      glEnable(GL_BLEND); glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
      glEnable(GL_LIGHT0); glDisable(GL_LIGHT1); glDisable(GL_LIGHT2);
      glEnable(GL_NORMALIZE); glDepthFunc(GL_LEQUAL); glShadeModel(GL_SMOOTH); glBlendFunc(GL_ONE, GL_ZERO);
      glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mat_diffuse);
      glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_no_specular);
      glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_no_shininess);
      glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
      glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
      glEnable(GL_COLOR_MATERIAL);
      glGenTextures(1, &(frame->data_texture)); frame->construct_data_slice();
      glGenTextures(1, &(frame->xz_texture)); frame->construct_xz_reslice();
      glGenTextures(1, &(frame->yz_texture)); frame->construct_yz_reslice();
      glGenTextures(1, &(frame->any_texture)); frame->construct_any_reslice();
      frame->tri_list = glGenLists(1);
      for (i=0; i<16; i++) scene_rotate[i] = 0.0;
      scene_rotate[0] = 1.0; scene_rotate[5] = -1.0; scene_rotate[10] = -1.0; scene_rotate[15] = 1.0; 
      scene_pan[0] = 0.0; scene_pan[1] = 0.0; scene_scale = 2.5;
      heading = 0.0; offset_x = 0.0; offset_z = -INITIAL_DEPTH_OFFSET; velocity_x = 0.0; velocity_z = 0.0;
    }

    // Set up the projection matrix for the 3D view
    if (frame->view_angle > 0) gluPerspective((float)frame->view_angle, (float)w/(float)h, PERSPECTIVE_VIEW_NEAR, PERSPECTIVE_VIEW_FAR);
    else glOrtho(-2*w, 2*w, -2*h, 2*h, 0.0, ORTHO_VIEW_DEPTH);

  } else { // the slice/reslice windows

    // Set up the projection matrix and the pixel drawing environment for the slice/reslice windows
    glOrtho(0, w, 0, h, -1, 1); 
    glRasterPos2d(PART_PIXEL, h - PART_PIXEL);
    glPixelZoom(1.0, -1.0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  }

}

void MyGLCanvas::OnSize(wxSizeEvent& event)
  // Callback function for when the canvas is resized
{
  configured = false; // this will force the viewport and projection matrices to be reconfigured on the next paint
}

void MyGLCanvas::OnMouse(wxMouseEvent& event)
  // Callback function for mouse events inside the GL canvas
{
  static int lastx, lasty;
  int x, y;

  // Make sure that the top left canvas has keyboard focus when the mouse is in any of the drawing areas
  if (event.Entering()) frame->tl_canvas->SetFocus();

  // Other mouse events to be processed only by the top left canvas - the 3D view window
  if (canvas_type != CANVAS_TL) return;

  if (event.ButtonDown()) { // start of a drag operation
    lastx = event.m_x; lasty = event.m_y;
  } else if (event.Dragging()) { // adjust the scene's pose
    x = event.GetX() - lastx;
    y = event.GetY() - lasty;
    SetCurrent(*context);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    if (event.m_leftDown) glRotatef(sqrtf(x*x + y*y), y, x, 0.0); // rotate scene around centroid
    if (event.m_middleDown) glRotatef((x+y), 0.0, 0.0, 1.0); // rotate scene around centroid
    if (event.m_rightDown) { // pan scene
      scene_pan[0] += 2*x;
      scene_pan[1] -= 2*y;
    }
    glMultMatrixf(scene_rotate);
    glGetFloatv(GL_MODELVIEW_MATRIX, scene_rotate);
    lastx = event.GetX();
    lasty = event.GetY();	
    Refresh();
  } else if (event.GetWheelRotation()!=0) { // the mouse wheel - enlarge or shrink the scene
    if (event.GetWheelRotation() < 0) scene_scale *= (1.0 - (float)event.GetWheelRotation()/(20*event.GetWheelDelta()));
    else scene_scale /= (1.0 + (float)event.GetWheelRotation()/(20*event.GetWheelDelta()));
    Refresh();
  }
}

void MyGLCanvas::OnKey(wxKeyEvent &event)
// Callback function for key presses inside the GL canvas
{
  
  // We always want to process wireframe key presses
  if ((event.GetKeyCode() == 'w') || (event.GetKeyCode() == 'W')) {
    wireframe = !wireframe;
    Refresh();
  }

  // We only want to process other key presses in fly-through mode
  if (!frame->fly) return;

  switch (event.GetKeyCode()) {
  case WXK_UP: 
    velocity_x += FLY_SPEED_STEP*sin(heading*M_PI/180.0)/frame->fps;
    velocity_z -= FLY_SPEED_STEP*cos(heading*M_PI/180.0)/frame->fps;
    break;
  case WXK_DOWN: 
    velocity_x -= FLY_SPEED_STEP*sin(heading*M_PI/180.0)/frame->fps;
    velocity_z += FLY_SPEED_STEP*cos(heading*M_PI/180.0)/frame->fps;
    break;
  case WXK_LEFT:
    heading -= FLY_ANGLE_STEP;
    break;
  case WXK_RIGHT:
    heading += FLY_ANGLE_STEP;
    break;
  case ' ': 
    velocity_x = 0.0;
    velocity_z = 0.0;
    break;
  default:
    break;
  }
}

void MyGLCanvas::OnPaint(wxPaintEvent& event)
  // Callback function for when the GL canvas is exposed
{
  wxPaintDC dc(this); // the wxWidgets documentation says we have to create a drawing context, even if we don't use it!

  SetCurrent(*context);
  if (!configured) {
    ConfigureGL();
    configured = true;
  }
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  switch (canvas_type) {
  case CANVAS_TR:
    glDrawPixels(VOLUME_WIDTH, VOLUME_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, frame->data_rgb);
    break;
  case CANVAS_BL:
    glDrawPixels(VOLUME_HEIGHT, VOLUME_DEPTH, GL_RGB, GL_UNSIGNED_BYTE, frame->yz_rgb);
    break;
  case CANVAS_BR:
    glDrawPixels(VOLUME_WIDTH, VOLUME_DEPTH, GL_RGB, GL_UNSIGNED_BYTE, frame->any_rgb);
    break;
  case CANVAS_TL:
    draw_view_window();
    break;
  default:
    break;
  }

  glFlush();
  SwapBuffers();
}

void MyGLCanvas::draw_surface_in_view(void)
// Draws the triangle mesh in the 3D view window
{
  // Switch lighting model for surface rendering
  glColor4f(0.0, 0.0, 0.0, 0.0);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glDisable(GL_LIGHT0);
  glEnable(GL_LIGHT1);
  glEnable(GL_LIGHT2);

  // Set the surface type and colour
  if (wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glColor4f(1.0, 0.7, 0.5, frame->surface_alpha/10.0);

  if (frame->surface_alpha == 10) {
    glDisable(GL_BLEND);
    glCallList(frame->tri_list);
    glEnable(GL_BLEND);
  } else { // draw the surfaces, back faces first to improve transparency effects
    glCullFace(GL_FRONT);
    glEnable(GL_CULL_FACE);
    glCallList(frame->tri_list);
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glCallList(frame->tri_list);
    glDisable(GL_CULL_FACE);
  }

  // Switch back to original lighting model
  if (wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glColor4f(0.0, 0.0, 0.0, 0.0);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_no_specular);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_no_shininess);
  glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
  glBlendFunc(GL_ONE, GL_ZERO);
  glEnable(GL_LIGHT0);
  glDisable(GL_LIGHT1);
  glDisable(GL_LIGHT2);
}

void MyGLCanvas::draw_reslice_in_view(bool texture_valid, GLuint texture, float image_width, float image_height, int tex_width, int tex_height, float colour[], float coords[])
// Draws the slice and reslices in the 3D view window
{
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  if (texture_valid) { // draw the texture map of the reslice image
    glColor3f(1.0, 1.0, 1.0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_POLYGON);
    glTexCoord2f(0.0, 0.0);
    glVertex3f(coords[0], coords[1], coords[2]);

    glTexCoord2f(image_width/tex_width, 0.0);
    glVertex3f(coords[3], coords[4], coords[5]);
    glTexCoord2f(image_width/tex_width, image_height/tex_height);
    glVertex3f(coords[6], coords[7], coords[8]);
    glTexCoord2f(0.0 , image_height/tex_height);
    glVertex3f(coords[9], coords[10], coords[11]);
    glEnd();
    glDisable(GL_TEXTURE_2D);
  } else { // draw the shaded interior of the reslice plane
    glColor3f(0.5*colour[0], 0.5*colour[1], 0.5*colour[2]);
    glBegin(GL_POLYGON);
    glVertex3f(coords[0], coords[1], coords[2]);
    glVertex3f(coords[3], coords[4], coords[5]);
    glVertex3f(coords[6], coords[7], coords[8]);
    glVertex3f(coords[9], coords[10], coords[11]);
    glEnd();
  }

  // Draw the solid border of the reslice plane
  glBlendFunc(GL_ONE, GL_ZERO);
  glColor3f(colour[0], colour[1], colour[2]);
  glBegin(GL_LINE_LOOP);
  glVertex3f(coords[0], coords[1], coords[2]);
  glVertex3f(coords[3], coords[4], coords[5]);
  glVertex3f(coords[6], coords[7], coords[8]);
  glVertex3f(coords[9], coords[10], coords[11]);
  glEnd();
}

void MyGLCanvas::draw_view_window(void)
// Renders the 3D view window
{
  float coords[12], colour[3];
  GLfloat alpha_diffuse[] = { 0.0, 0.0, 0.0, 0.0 };
  bool fly = frame->fly;
  bool mobile_lights = frame->mobile_lights;

  // Set the appropriate alpha value for blending
  alpha_diffuse[3] = frame->reslice_alpha/10.0;
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, alpha_diffuse);

  // Initialise the modelling transformation
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glRotatef(heading, 0, 1, 0);
  glTranslatef(offset_x, 0.0, offset_z);


  if (fly) {

    // Update the view position
    offset_x -= velocity_x;
    offset_z -= velocity_z;
    
    // EXERCISE 3
    // Adjust the modelview matrix to represent the current displacement between the viewpoint
    // and the scene. The translational displacement is (offset_x, 0.0, offset_z), while
    // the rotation around the y-axis is heading. Also, set up GL_LIGHT1 and GL_LIGHT2 correctly,
    // according to the current value of the boolean variable mobile_lights.

  } else {

    // Viewing transformation - set the viewpoint back from the objects in the scene
    glTranslatef(0.0, 0.0, -INITIAL_DEPTH_OFFSET);

  }

  // Apply the modelling transformation
  glTranslatef(scene_pan[0], scene_pan[1], 0.0);
  glMultMatrixf(scene_rotate);
  glScalef(scene_scale, scene_scale, scene_scale);
  glTranslatef(-VOLUME_WIDTH/2.0, -VOLUME_HEIGHT/2.0, -VOLUME_DEPTH/2.0);

  // The ordering of what follows matters in the context of alpha blending

  if (frame->show_reslices) {

    // Draw the data slice
    colour[0] = 0.0; colour[1] = 0.0; colour[2] = 1.0;
    coords[0] = 0.0; coords[1] = 0.0; coords[2] = frame->data_slice_number * SLICE_SEPARATION;
    coords[3] = VOLUME_WIDTH; coords[4] = 0.0; coords[5] = frame->data_slice_number * SLICE_SEPARATION;
    coords[6] = VOLUME_WIDTH; coords[7] = VOLUME_HEIGHT; coords[8] = frame->data_slice_number * SLICE_SEPARATION;
    coords[9] = 0.0; coords[10] = VOLUME_HEIGHT; coords[11] = frame->data_slice_number * SLICE_SEPARATION;
    draw_reslice_in_view(frame->data_texture_valid, frame->data_texture, VOLUME_WIDTH, VOLUME_HEIGHT, frame->data_tex_width, frame->data_tex_height, colour, coords);
    
    // Draw the x-z reslice
    colour[0] = 1.0; colour[1] = 0.0; colour[2] = 0.0;
    coords[0] = 0.0; coords[1] = frame->y_reslice_position; coords[2] = 0.0;
    coords[3] = VOLUME_WIDTH; coords[4] = frame->y_reslice_position; coords[5] = 0.0;
    coords[6] = VOLUME_WIDTH; coords[7] = frame->y_reslice_position; coords[8] = VOLUME_DEPTH;
    coords[9] = 0.0; coords[10] = frame->y_reslice_position; coords[11] = VOLUME_DEPTH;
    draw_reslice_in_view(frame->xz_texture_valid, frame->xz_texture, VOLUME_WIDTH, VOLUME_DEPTH, frame->xz_tex_width, frame->xz_tex_height, colour, coords);
    
    // Draw the y-z reslice
    colour[0] = 0.0; colour[1] = 1.0; colour[2] = 0.0;
    coords[0] = frame->x_reslice_position; coords[1] = 0.0; coords[2] = 0.0;
    coords[3] = frame->x_reslice_position; coords[4] = VOLUME_HEIGHT; coords[5] = 0.0;
    coords[6] = frame->x_reslice_position; coords[7] = VOLUME_HEIGHT; coords[8] = VOLUME_DEPTH;
    coords[9] = frame->x_reslice_position; coords[10] = 0; coords[11] = VOLUME_DEPTH;
    draw_reslice_in_view(frame->yz_texture_valid, frame->yz_texture, VOLUME_HEIGHT, VOLUME_DEPTH, frame->yz_tex_width, frame->yz_tex_height, colour, coords);

    // Draw the any reslice (in purple)
    // TODO come back and put in the coords for the polygon based apon the 
    colour[0] = 0.5; colour[1] = 0; colour[2] = 0.7;
    coords[0] = frame->x_reslice_position; coords[1] = 0.0; coords[2] = 0.0;
    coords[3] = frame->x_reslice_position; coords[4] = frame->y_reslice_position; coords[5] = 0.0;
    coords[6] = frame->x_reslice_position; coords[7] = frame->y_reslice_position; coords[8] = VOLUME_DEPTH;
    coords[9] = frame->x_reslice_position; coords[10] = frame->y_reslice_position; coords[11] = VOLUME_DEPTH;
    // draw the pixels within the slice
    draw_reslice_in_view(frame->any_texture_valid, frame->any_texture, VOLUME_HEIGHT, VOLUME_DEPTH, frame->any_tex_width, frame->any_tex_height, colour, coords);

    
  }

  // Draw the surface
  if (frame->show_surface) draw_surface_in_view();
}


// MyFrame ///////////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
  EVT_MENU(wxID_EXIT, MyFrame::OnQuit)
  EVT_MENU(wxID_ABOUT, MyFrame::OnAbout)
  EVT_COMMAND_SCROLL(VA_SLIDER_ID, MyFrame::OnVASlider)
  EVT_COMMAND_SCROLL(RO_SLIDER_ID, MyFrame::OnROSlider)
  EVT_COMMAND_SCROLL(SO_SLIDER_ID, MyFrame::OnSOSlider)
  EVT_COMMAND_SCROLL(XY_SLIDER_ID, MyFrame::OnXYSlider)
  EVT_COMMAND_SCROLL(XZ_SLIDER_ID, MyFrame::OnXZSlider)
  EVT_COMMAND_SCROLL(YZ_SLIDER_ID, MyFrame::OnYZSlider)
  EVT_COMMAND_SCROLL(THETA_ID, MyFrame::OnTHETASlider)
  EVT_COMMAND_SCROLL(PHI_ID, MyFrame::OnPHISlider)
  EVT_COMMAND_SCROLL(PSI_ID, MyFrame::OnPSISlider)
  EVT_COMMAND_SCROLL(ST_SLIDER_ID, MyFrame::OnSTSlider)
  EVT_COMMAND_SCROLL(SR_SLIDER_ID, MyFrame::OnSRSlider)
  EVT_CHECKBOX(INTERP_CHECKBOX_ID, MyFrame::OnInterp)
  EVT_CHECKBOX(LIGHTS_CHECKBOX_ID, MyFrame::OnLights)
  EVT_CHECKBOX(SLICE_CHECKBOX_ID, MyFrame::OnSlice)
  EVT_CHECKBOX(SURFACE_CHECKBOX_ID, MyFrame::OnSurface)
  EVT_CHECKBOX(FLY_CHECKBOX_ID, MyFrame::OnFly)
  EVT_BUTTON(ISO_BUTTON_ID, MyFrame::OnCalcIso)
  EVT_CLOSE(MyFrame::OnClose)
END_EVENT_TABLE()
  
MyFrame::MyFrame(wxWindow *parent, const wxString& title, wxString filename, const wxPoint& pos, const wxSize& size, long style):
  wxFrame(parent, wxID_ANY, title, pos, size, style)
  // Constructor - loads the data, lays out the canvases and controls using sizers
{
  // Load the data
  wxFile fid;
  bool read_in = false;
  if (wxFile::Access(filename, wxFile::read)) {
    if (fid.Open(filename)) {
      if (fid.Length() ==  VOLUME_WIDTH * VOLUME_HEIGHT * NUM_SLICES) {
	fid.Read(data, fid.Length());
	read_in = true;
      }
    }
  }
  if (!read_in) {
    wcout << "Invalid data file" << endl;
    exit(1);
  }

  // Lay out the canvases and controls
  SetIcon(wxIcon(wx_icon));
  wxMenu *fileMenu = new wxMenu;
  fileMenu->Append(wxID_ABOUT, "&About");
  fileMenu->Append(wxID_EXIT, "&Quit");
  wxMenuBar *menuBar = new wxMenuBar;
  menuBar->Append(fileMenu, "&File");
  SetMenuBar(menuBar);
  wxStatusBar *statusBar = CreateStatusBar(2);
  int widths[2] = {-2, -1};
  statusBar->SetStatusWidths(2, widths);
  int styles[2] = {wxSB_SUNKEN, wxSB_SUNKEN};
  statusBar->SetStatusStyles(2, styles);

  // The window is layed out using topsizer, inside which are lower level windows and sizers for the canvases and controls
  wxBoxSizer *topsizer = new wxBoxSizer(wxHORIZONTAL); SetSizer(topsizer);
  wxBoxSizer *canvas_h_sizer = new wxBoxSizer(wxHORIZONTAL);
  wxBoxSizer *canvas_v1_sizer = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer *canvas_v2_sizer = new wxBoxSizer(wxVERTICAL);
  wxScrolledWindow* controlwin = new wxScrolledWindow(this, -1, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL);
  topsizer->Add(canvas_h_sizer, 1, wxEXPAND);
  topsizer->Add(controlwin, 0, wxEXPAND);
  canvas_h_sizer->Add(canvas_v1_sizer, 1, wxEXPAND);
  canvas_h_sizer->Add(canvas_v2_sizer, 1, wxEXPAND);

  // Create the four canvases and add them to the relevant sizer
  tl_canvas = new MyGLCanvas(this, wxID_ANY, CANVAS_TL);
  tr_canvas = new MyGLCanvas(this, wxID_ANY, CANVAS_TR);
  bl_canvas = new MyGLCanvas(this, wxID_ANY, CANVAS_BL);
  br_canvas = new MyGLCanvas(this, wxID_ANY, CANVAS_BR);
  canvas_v1_sizer->Add(tl_canvas, 1, wxEXPAND | wxALL, 2);
  canvas_v1_sizer->Add(bl_canvas, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 2);
  canvas_v2_sizer->Add(tr_canvas, 1, wxEXPAND | wxTOP | wxBOTTOM, 2);
  canvas_v2_sizer->Add(br_canvas, 1, wxEXPAND | wxBOTTOM, 2);

  // Create a sizer to manage the contents of the scrolled control window
  wxBoxSizer *control_sizer = new wxBoxSizer(wxVERTICAL);
  controlwin->SetSizer(control_sizer);
  controlwin->SetScrollRate(10, 10);
  controlwin->SetMinSize(wxSize(210,-1));

  // Create the rendering controls and add them to the control sizer
  show_reslices = true;
  wxCheckBox *slice_cb = new wxCheckBox(controlwin, SLICE_CHECKBOX_ID, "show slice and reslices");
  control_sizer->Add(slice_cb, 0, wxALL, 5);
  slice_cb->SetValue(show_reslices);

  show_surface = false;
  wxCheckBox *surface_cb = new wxCheckBox(controlwin, SURFACE_CHECKBOX_ID, "show surface");
  control_sizer->Add(surface_cb, 0, wxALL, 5);
  surface_cb->SetValue(show_surface);

  mobile_lights = false;
  wxCheckBox *lights_cb = new wxCheckBox(controlwin, LIGHTS_CHECKBOX_ID, "lights move with viewer");
  control_sizer->Add(lights_cb, 0, wxALL, 5);
  lights_cb->SetValue(mobile_lights);

  fly = false;
  wxCheckBox *fly_cb = new wxCheckBox(controlwin, FLY_CHECKBOX_ID, "fly-through");
  control_sizer->Add(fly_cb, 0, wxALL, 5);
  fly_cb->SetValue(fly);

  view_angle = 0;
  control_sizer->Add(new wxStaticText(controlwin, wxID_ANY, "view angle (degrees)"), 0, wxTOP | wxLEFT, 10);
  control_sizer->Add(new wxSlider(controlwin, VA_SLIDER_ID, view_angle, 0, 135, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL | wxSL_VALUE_LABEL), 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);

  reslice_alpha = 5;
  control_sizer->Add(new wxStaticText(controlwin, wxID_ANY, "reslice opacity"), 0, wxTOP | wxLEFT, 10);
  control_sizer->Add(new wxSlider(controlwin, RO_SLIDER_ID, reslice_alpha, 0, 10, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL | wxSL_VALUE_LABEL), 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);
  surface_alpha = 10;
  control_sizer->Add(new wxStaticText(controlwin, wxID_ANY, "surface opacity"), 0, wxTOP | wxLEFT, 10);
  control_sizer->Add(new wxSlider(controlwin, SO_SLIDER_ID, surface_alpha, 0, 10, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL | wxSL_VALUE_LABEL), 0,  wxLEFT | wxRIGHT | wxBOTTOM, 5);

  // Separate the rendering controls from the reslice controls
  control_sizer->AddSpacer(45);

  // Create the reslice controls and add them to the control sizer
  interpolate = true;
  wxCheckBox *interp_cb = new wxCheckBox(controlwin, INTERP_CHECKBOX_ID, "linear interpolation");
  control_sizer->Add(interp_cb, 0, wxALL, 5);
  interp_cb->SetValue(interpolate);

  data_slice_number = NUM_SLICES/2.0;
  control_sizer->Add(new wxStaticText(controlwin, wxID_ANY, "x-y data slice"), 0, wxLEFT, 10);
  control_sizer->Add(new wxSlider(controlwin, XY_SLIDER_ID, data_slice_number, 0, NUM_SLICES-1, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL), 0, wxALL, 5);

  y_reslice_position = VOLUME_HEIGHT/2.0;
  control_sizer->Add(new wxStaticText(controlwin, wxID_ANY, "x-z reslice"), 0, wxLEFT, 10);
  control_sizer->Add(new wxSlider(controlwin, XZ_SLIDER_ID, y_reslice_position, 0, VOLUME_HEIGHT-1, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL), 0, wxALL, 5);

  x_reslice_position = VOLUME_WIDTH/2.0;
  control_sizer->Add(new wxStaticText(controlwin, wxID_ANY, "y-z reslice"), 0, wxLEFT, 10);
  control_sizer->Add(new wxSlider(controlwin, YZ_SLIDER_ID, x_reslice_position, 0, VOLUME_WIDTH-1, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL), 0, wxALL, 5);

  // Separate the reslice controls from the isosurface controls
  control_sizer->AddSpacer(45);

  // Separate the reslice controls from the angle reslice controls
  control_sizer->AddSpacer(45);

  // Create the angle reslice controls and add them to the control sizer

  theta = 0;
  control_sizer->Add(new wxStaticText(controlwin, wxID_ANY, "theta"), 0, wxLEFT, 360);
  control_sizer->Add(new wxSlider(controlwin, THETA_ID, theta, 0, 360, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL), 0, wxALL, 5);

  phi = 0;
  control_sizer->Add(new wxStaticText(controlwin, wxID_ANY, "phi"), 0, wxLEFT, 360);
  control_sizer->Add(new wxSlider(controlwin, PHI_ID, phi, 0, 360, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL), 0, wxALL, 5);

  psi = 0;
  control_sizer->Add(new wxStaticText(controlwin, wxID_ANY, "psi"), 0, wxLEFT, 360);
  control_sizer->Add(new wxSlider(controlwin, PSI_ID, psi, 0, 360, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL), 0, wxALL, 5);



  // Create the isosurface controls and add them to the control sizer
  threshold = 255;
  set_threshold_colourmap();
  control_sizer->Add(new wxStaticText(controlwin, wxID_ANY, "surface threshold"), 0, wxTOP | wxLEFT, 10);
  control_sizer->Add(new wxSlider(controlwin, ST_SLIDER_ID, threshold, 0, 255, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL | wxSL_VALUE_LABEL), 0,  wxLEFT | wxRIGHT | wxBOTTOM, 5);

  surface_resolution = 5;
  control_sizer->Add(new wxStaticText(controlwin, wxID_ANY, "surface resolution"), 0, wxTOP | wxLEFT, 10);
  control_sizer->Add(new wxSlider(controlwin, SR_SLIDER_ID, surface_resolution, 1, 40, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL | wxSL_VALUE_LABEL), 0,  wxLEFT | wxRIGHT | wxBOTTOM, 5);
  control_sizer->Add(new wxButton(controlwin, ISO_BUTTON_ID, "Calculate isosurface"), 0, wxALIGN_CENTRE | wxALL, 5);
}

void MyFrame::OnClose(wxCloseEvent &event)
  // Called when application is closed down (including window manager close)
{
  Destroy();
  exit(0); // a workaround for a wxWidgets bug, should be able to remove this once the wxWidgets bug is fixed
}

void MyFrame::OnQuit(wxCommandEvent &event)
  // Callback for the "Quit" menu item
{
  Close(true);
}

void MyFrame::OnAbout(wxCommandEvent &event)
  // Callback for the "About" menu item
{
  wxMessageDialog about(this, "Laboratory 3G4 (wxWidgets version)\nAndrew Gee\nJanuary 2016", "About Laboratory 3G4", wxICON_INFORMATION | wxOK);
  about.ShowModal();
}

void MyFrame::OnROSlider(wxScrollEvent &event)
  // Callback for the reslice opacity slider
{
  reslice_alpha = event.GetPosition();
  tl_canvas->Refresh();
}

void MyFrame::OnSOSlider(wxScrollEvent &event)
  // Callback for the surface opacity slider
{
  surface_alpha = event.GetPosition();
  tl_canvas->Refresh();
}

void MyFrame::OnSRSlider(wxScrollEvent &event)
  // Callback for the surface resolution slider
{
  surface_resolution = event.GetPosition();
}

void MyFrame::OnSTSlider(wxScrollEvent &event)
// Callback for the surface threshold slider
{
  threshold = event.GetPosition();
  set_threshold_colourmap();
  construct_data_slice();
  construct_xz_reslice();
  construct_yz_reslice();
  construct_any_reslice();
  tl_canvas->Refresh();
  tr_canvas->Refresh();
  bl_canvas->Refresh();
  br_canvas->Refresh();
}

void MyFrame::OnVASlider(wxScrollEvent &event)
  // Callback for the view angle slider
{
  view_angle = event.GetPosition();
  tl_canvas->configured = false; // this will cause the projection matrix to be reconfigured
  tl_canvas->Refresh();
}

void MyFrame::OnXYSlider(wxScrollEvent &event)
  // Callback for the XY slider
{
  data_slice_number = event.GetPosition();
  construct_data_slice();
  tl_canvas->Refresh();
  tr_canvas->Refresh();
}

void MyFrame::OnXZSlider(wxScrollEvent &event)
  // Callback for the XZ slider
{
  y_reslice_position = event.GetPosition();
  construct_xz_reslice();
  construct_any_reslice();
  tl_canvas->Refresh();
  br_canvas->Refresh();
}

void MyFrame::OnYZSlider(wxScrollEvent &event)
  // Callback for the YZ slider
{
  x_reslice_position = event.GetPosition();
  construct_yz_reslice();
  construct_any_reslice();
  tl_canvas->Refresh();
  bl_canvas->Refresh();
}

void MyFrame::OnTHETASlider(wxScrollEvent &event)
  // Callback for the theta slider
{
  theta = event.GetPosition();
  construct_any_reslice();
  tl_canvas->Refresh();
  bl_canvas->Refresh();
}

void MyFrame::OnPHISlider(wxScrollEvent &event)
  // Callback for the phi slider
{
  phi = event.GetPosition();
  construct_any_reslice();
  tl_canvas->Refresh();
  bl_canvas->Refresh();
}

void MyFrame::OnPSISlider(wxScrollEvent &event)
  // Callback for the psi slider
{
  psi = event.GetPosition();
  construct_any_reslice();
  tl_canvas->Refresh();
  bl_canvas->Refresh();
}

void MyFrame::OnSlice(wxCommandEvent &event)
  // Callback for the slice/reslice checkbox
{
  show_reslices = event.IsChecked();
  tl_canvas->Refresh();
}

void MyFrame::OnLights(wxCommandEvent &event)
  // Callback for the mobile lights checkbox
{
  mobile_lights = event.IsChecked();
  tl_canvas->Refresh();
}

void MyFrame::OnSurface(wxCommandEvent &event)
  // Callback for the surface checkbox
{
  show_surface = event.IsChecked();
  tl_canvas->Refresh();
}

void MyFrame::OnFly(wxCommandEvent &event)
  // Callback for the surface checkbox
{
  fly = event.IsChecked();
  if (fly) {
    wxTheApp->Connect(wxID_ANY, wxEVT_IDLE, wxIdleEventHandler(MyApp::OnIdle));
    SetStatusText("Estimating frame rate", 1);
    fps = 25; // initial frame per second estimate
  }
  else {
    wxTheApp->Disconnect(wxID_ANY, wxEVT_IDLE, wxIdleEventHandler(MyApp::OnIdle));
    SetStatusText(' ', 1);
    // Reset the viewpoint and lights
    tl_canvas->heading = 0.0;
    tl_canvas->offset_x = 0.0;
    tl_canvas->offset_z = -INITIAL_DEPTH_OFFSET;
    tl_canvas->velocity_x = 0.0;
    tl_canvas->velocity_z = 0.0;
    tl_canvas->SetCurrent(*tl_canvas->context);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glLightfv(GL_LIGHT1, GL_POSITION, top_right);
    glLightfv(GL_LIGHT2, GL_POSITION, straight_on);
    tl_canvas->Refresh();
  }
}

void MyFrame::OnCalcIso(wxCommandEvent &event)
  // Callback for the calculate isosurface button
{
  tl_canvas->SetCurrent(*tl_canvas->context);
  tris = isosurf(VOLUME_WIDTH, VOLUME_HEIGHT, NUM_SLICES, (float)surface_resolution, SLICE_SEPARATION, data, threshold, tri_list, n_tris);
  SetStatusText(wxString::Format("Surface comprises %d triangles, area %f square cm, volume %f ml", 
				 n_tris, mesh_area()*PIXEL_DIMENSION*PIXEL_DIMENSION*10000, mesh_volume()*PIXEL_DIMENSION*PIXEL_DIMENSION*PIXEL_DIMENSION*1000000));
  tl_canvas->Refresh();
}

void MyFrame::OnInterp(wxCommandEvent &event)
  // Callback for the linear interpolation checkbox
{
  interpolate = event.IsChecked();
  construct_xz_reslice();
  construct_yz_reslice();
  tl_canvas->Refresh();
  bl_canvas->Refresh();
  br_canvas->Refresh();
}

void MyFrame::set_threshold_colourmap(void)
// Populates the colourmaps for tha given threshold
{
  int i;

  for(i=0; i<256; i++)
    if (i > threshold) {
      MapItoR[i]=255; MapItoG[i]=0; MapItoB[i]=255;
    } else {
      MapItoR[i]=i; MapItoG[i]=i; MapItoB[i]=i;
    }generate_texture
}

bool generate_texturedth, int &tex_height, GLuint texture, unsigned char *image)
// Generates the texture maps for displaying the slice/reslice images in the 3D view window
{
  unsigned char *tex_image, *in, *out;
  int x, y;

  // Initialise tex_width and _height depending on size of image - must be 2^n
  tex_width = 1; while (tex_width < image_width) tex_width <<= 1;
  tex_height = 1; while (tex_height < image_height) tex_height <<= 1;
  if ( NULL == (tex_image = (unsigned char*) malloc(3 * tex_width * tex_height * sizeof(unsigned char))) ) exit(1);

  // Copy the image into the texture map
  in = image;
  out = tex_image;
  for (y=0; y<tex_height; y++)
    for (x=0; x<tex_width; x++) {
      if (x<image_width && y<image_height) {
	*out++=*in++;
	*out++=*in++;
	*out++=*in++;
      } else {
	*out++=0;
	*out++=0;
	*out++=0;
      }
    }
  
  // Bind the texture to the OpenGL drawing context
  tl_canvas->SetCurrent(*tl_canvas->context);
  glBindTexture(GL_TEXTURE_2D, texture);
  glGetError(); // clear error
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_width, tex_height, 0, GL_RGB, GL_UNSIGNED_BYTE, tex_image);
  free(tex_image);
  if (glGetError() == GL_NO_ERROR) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    return true;
  } else return false;

}

void MyFrame::construct_data_slice(void)
// Construct the data slice image
{
  int x, y, data_index, slice_index;

  data_index = (int) data_slice_number * VOLUME_WIDTH * VOLUME_HEIGHT;
  slice_index = 0;

  // Simply extract the appropriate slice from the 3D data
  for (y=0; y<VOLUME_HEIGHT; y++) {
    for (x=0; x<VOLUME_WIDTH; x++) {
      data_rgb[slice_index++] = MapItoR[ data[data_index] ];
      data_rgb[slice_index++] = MapItoG[ data[data_index] ];
      data_rgb[slice_index++] = MapItoB[ data[data_index] ];
      data_index++;
    }
  }

  // Bind the new reslice image to the texture map in the view window
  data_texture_valid = generate_texture(VOLUME_WIDTH, VOLUME_HEIGHT, data_tex_width, data_tex_height, data_texture, data_rgb);
}

void MyFrame::construct_xz_reslice(void)
// Construct the x-z reslice image
{
  if (interpolate) construct_xz_reslice_linear();
  else construct_xz_reslice_nearest_neighbour();

  // Bind the new reslice image to the texture map in the view window
  xz_texture_valid = generate_texture(VOLUME_WIDTH, VOLUME_DEPTH, xz_tex_width, xz_tex_height, xz_texture, xz_rgb);
}

void MyFrame::construct_yz_reslice(void)
// Construct the y-z reslice image
{
  if (interpolate) construct_yz_reslice_linear();
  else construct_yz_reslice_nearest_neighbour();

  // Bind the new reslice image to the texture map in the view window
  yz_texture_valid = generate_texture(VOLUME_HEIGHT, VOLUME_DEPTH, yz_tex_width, yz_tex_height, yz_texture, yz_rgb);
}

void MyFrame::construct_any_reslice(void)
// Construct the any angle reslice image
{
  construct_any_reslice_nearest_neighbour();


  // Bind the new reslice image to the texture map in the view window
  any_texture_valid = generate_texture(VOLUME_HEIGHT, VOLUME_DEPTH, any_tex_width, any_tex_height, any_texture, any_rgb);
}

void MyFrame::construct_any_reslice_nearest_neighbour(void)
// 
// This function currently constructs a blank any angle reslice. Modify it so it constructs
// a valid reslice according to the current x_reslice_position, y_reslice_position, and angles theta, phi and psi. Use nearest neighbour
// interpolation.
{
  int max_traversal, z_counter, x_counter, data_index, reslice_index;
  double xx_advance, zx_advance, xy_advance, zy_advance, zz_advance, xz_advance, x_pos, x_neg, xy_pos, xy_neg, zy_pos, zy_neg, z_pos, z_neg;
  bool x_pos_valid, x_neg_valid, z_pos_valid, z_neg_valid, y_pos_pos_valid, y_pos_neg_valid, y_neg_pos_valid, y_neg_neg_valid;

  reslice_index = 0;
  // Plane orientations (+ve is anticlockwise)

  /// global (volume data) movements
  // movement in x direction due to movement in reslice x direction
  xx_advance = cos(psi) * cos(phi);
  // movement in x direction due to movement in reslice z direction
  zx_advance = cos(theta) * sin(phi);
  // movement in y direction due to movement in reslice x direction
  xy_advance = sin(psi);
  // movement in y direction due to movement in reslice z direction
  zy_advance = -sin(theta);
  // movement in z direction due to movement in reslice z direction
  zz_advance = cos(theta) * cos(phi);
  // movement in z direction due to movement in reslice x direction
  xz_advance = cos(psi) * sin(-phi);
  
  /// initialise the volume positions at plane origin
  /// pos and neg are simply notation for which quarter plane these
  /// values reside in and should infact always be +ve as they lie
  /// in the volumetric data space but may temporarily fall outside
  /// depending on plane definition
  x_pos -= x_neg = x_reslice_position;
  xy_pos = zy_pos = xy_neg = zy_neg = y_reslice_position;
  z_pos =  z_neg = data_slice_number;

  max_traversal = ceil(sqrt(VOLUME_WIDTH*VOLUME_WIDTH + VOLUME_HEIGHT*VOLUME_HEIGHT + VOLUME_DEPTH*VOLUME_DEPTH));
  
  //four reslice regions and the corresponding colour mapped data
  int posx_posz_reslice[max_traversal][max_traversal];
  int posx_negz_reslice[max_traversal][max_traversal];
  int negx_posz_reslice[max_traversal][max_traversal];
  int negx_negz_reslice[max_traversal][max_traversal];
  unsigned char posx_posz_reslice_rgb[VOLUME_HEIGHT * VOLUME_DEPTH * 3];
  unsigned char posx_negz_reslice_rgb[VOLUME_HEIGHT * VOLUME_DEPTH * 3];
  unsigned char negx_posz_reslice_rgb[VOLUME_HEIGHT * VOLUME_DEPTH * 3];
  unsigned char negx_negz_reslice_rgb[VOLUME_HEIGHT * VOLUME_DEPTH * 3];
  /// region validty
  x_pos_valid = false;
  x_neg_valid = false;
  z_pos_valid = false;
  z_neg_valid = false;

  /// y requires a unique validity check for all four regions
  y_pos_pos_valid = false;
  y_pos_neg_valid = false;
  y_neg_pos_valid = false;
  y_neg_neg_valid = false;

  /// initialise reslice quarter plane counters at 0
  x_counter = z_counter = 0;

  // z reslice advancement loop
  for (z_counter=0; z_counter<max_traversal; z_counter++) {
    x_pos += zx_advance;
    x_neg -= zx_advance;

    zy_pos += zy_advance;
    zy_neg -= zy_advance;

    z_pos += zz_advance;
    z_neg -= zz_advance;

    // x reslice advancement loop
    for (x_counter=0; x_counter<max_traversal; x_counter++) {
      //advance volume positions due to movement along x reslice direction
      x_pos += xx_advance;
      x_neg -= xx_advance;

      xy_pos += xy_advance;
      xy_neg -= xy_advance;

      z_pos += xz_advance;
      z_neg -= xz_advance;

      // determine if current quarter volume coordinates are currently within volume
      // in x_pos domain
      if (x_pos > VOLUME_WIDTH || x_pos < 0) {
        x_pos_valid = false;
      }
      else{
        x_pos_valid = true;
      }
      //in x_neg domain
      if (x_neg > VOLUME_WIDTH || x_neg < 0) {
        x_neg_valid = false;
      }
      else{
        x_neg_valid = true;
      }
      //in z_pos domain
      if (z_pos > VOLUME_DEPTH || z_pos < 0) {
        z_pos_valid = false;
      }
      else{
        z_pos_valid = true;
      }
      if (xy_pos + zy_pos > VOLUME_HEIGHT || xy_pos + zy_pos < 0){
        y_pos_pos_valid = false;
      }
      else{
       y_pos_pos_valid = true;
      }
      //in y_pos_neg domain
      if (xy_pos + zy_neg > VOLUME_HEIGHT || xy_pos + zy_neg < 0){
       y_pos_neg_valid = false;
      }
      else{
       y_pos_neg_valid = true;
      }

      //in y_neg_pos domain
      if (xy_neg + zy_pos > VOLUME_HEIGHT || xy_neg + zy_pos < 0){
       y_neg_pos_valid = false;
      }
      else{
       y_neg_pos_valid = true;
      }
      //in y_neg_neg domain
      if (xy_neg + zy_neg > VOLUME_HEIGHT || xy_neg + zy_neg < 0){
       y_neg_neg_valid = false;
      }
      else{
       y_neg_neg_valid = true;
      }
      /// now determine points to transfer to reslice
      // pos pos region
      if (x_pos_valid && z_pos_valid && y_pos_pos_valid) {
        //valid volume coords so pass volume data to reslice
        int x_vol = round(x_pos);
        int y_vol = round(xy_pos + zy_pos);
        // nearest neighbour interpolation
        int z_vol = round(z_pos/SLICE_SEPARATION);

        data_index = z_vol*VOLUME_WIDTH*VOLUME_HEIGHT + y_vol*VOLUME_WIDTH + x_vol;
        // colour transform can occur here also
        posx_posz_reslice[z_counter][x_counter] = data[data_index];
        
        any_rgb[reslice_index++] = MapItoR[data[data_index]];
        any_rgb[reslice_index++] = MapItoG[data[data_index]];
        any_rgb[reslice_index++] = MapItoB[data[data_index]];

      }
      else{
        // otherwise just fill with empty as point lies outside of data range
       posx_posz_reslice[z_counter][x_counter] = 0;
      }
      //pos neg region
      if (x_pos_valid && z_neg_valid && y_pos_neg_valid){
        // valid volume coords so pass volume data to reslice
        int x_vol = round(x_pos);
        int y_vol = round(xy_pos + zy_neg);
        int z_vol = round(z_neg/SLICE_SEPARATION);

        data_index = z_vol*VOLUME_WIDTH*VOLUME_HEIGHT + y_vol*VOLUME_WIDTH + x_vol;
        // colour transform can occur here also
        posx_negz_reslice[z_counter][x_counter] = data[data_index];

        any_rgb[reslice_index++] = MapItoR[data[data_index]];
        any_rgb[reslice_index++] = MapItoG[data[data_index]];
        any_rgb[reslice_index++] = MapItoB[data[data_index]];
      }
      else{
        // otherwise just fill with empty as point lies outside of data range
       posx_negz_reslice[z_counter][x_counter] = 0;
      }
      //neg pos region
      if (x_neg_valid && z_pos_valid && y_neg_pos_valid){
        // valid volume coords so pass volume data to reslice
        int x_vol = round(x_neg);
        int y_vol = round(xy_neg+zy_pos);
        int z_vol = round(z_pos/SLICE_SEPARATION);
        
        data_index = z_vol*VOLUME_WIDTH*VOLUME_HEIGHT + y_vol*VOLUME_WIDTH + x_vol;
        // colour transform can occur here also
        negx_posz_reslice[z_counter][x_counter] = data[data_index];
        any_rgb[reslice_index++] = MapItoR[data[data_index]];
        any_rgb[reslice_index++] = MapItoG[data[data_index]];
        any_rgb[reslice_index++] = MapItoB[data[data_index]];
      }
      else{
        // otherwise just fill with empty as point lies outside of data range
       negx_posz_reslice[z_counter][x_counter] = 0;
        // neg neg region
      }
      if (x_neg_valid && z_neg_valid && y_neg_neg_valid){
       // valid volume coords so pass volume data to reslice
        int x_vol = round(x_neg);
        int y_vol = round(xy_neg+zy_neg);
        int z_vol = round(z_neg/SLICE_SEPARATION);

        data_index = z_vol*VOLUME_WIDTH*VOLUME_HEIGHT + y_vol*VOLUME_WIDTH + x_vol;
        // colour transform can occur here also
        negx_negz_reslice[z_counter][x_counter] = data[data_index];
        any_rgb[reslice_index++] = MapItoR[data[data_index]];
        any_rgb[reslice_index++] = MapItoG[data[data_index]];
        any_rgb[reslice_index++] = MapItoB[data[data_index]];
      }
      else{
       // otherwise just fill with empty as point lies outside of data range
       negx_negz_reslice[z_counter][x_counter] = 0;
      }
    }
  }
  // Now concatinate regions
  // clip edges by removing empty regions
  
}

void MyFrame::construct_yz_reslice_nearest_neighbour(void)
// EXERCISE 1
// This function currently constructs a blank y-z reslice. Modify it so it constructs
// a valid reslice according to the current x_reslice_position. Use nearest neighbour
// interpolation.
{
  int z, y, nearest_slice, data_index, reslice_index, x_reslice_position_int;

  x_reslice_position_int = round(x_reslice_position);
  reslice_index = 0;
  for (z=0; z<VOLUME_DEPTH; z++) {
    nearest_slice = round(z/SLICE_SEPARATION);
    data_index = nearest_slice * VOLUME_WIDTH * VOLUME_HEIGHT + x_reslice_position_int;
    for (y=0; y<VOLUME_HEIGHT; y++) {
      yz_rgb[reslice_index++] = MapItoR[data[data_index]];
      yz_rgb[reslice_index++] = MapItoG[data[data_index]];
      yz_rgb[reslice_index++] = MapItoB[data[data_index]];
      data_index += VOLUME_WIDTH;
    }
  }
}

void MyFrame::construct_xz_reslice_nearest_neighbour(void)
// EXERCISE 1
// This function currently constructs a blank x-z reslice. Modify it so it constructs
// a valid reslice according to the current y_reslice_position. Use nearest neighbour
// interpolation.
{
  int x, z, nearest_slice, data_index, reslice_index;

  reslice_index = 0;
  for (z=0; z<VOLUME_DEPTH; z++) {
    // nearest_slice = ...
    // data_index = ...
    for (x=0; x<VOLUME_WIDTH; x++) {
      xz_rgb[reslice_index++] = MapItoR[0];
      xz_rgb[reslice_index++] = MapItoG[0];
      xz_rgb[reslice_index++] = MapItoB[0];
      // data_index += ...
    }
  }
}

double MyFrame::mesh_volume(void)
// EXERCISE 2
// Write a function to calculate the volume enclosed by the isosurface.
// The function should return the volume, instead of the default value (zero) 
// that it currently returns. The units should be cubic pixels.
{
  return 0.0;
}

double MyFrame::mesh_area(void)
// Calculates surface area of mesh in units of square pixels - note area of triangle abc = 0.5 * | ab x ac |
{
  point ab, ac;
  double x, y, z, area;
  int t;

  area = 0.0;

  for (t=0; t<n_tris; t++) {
    // Calculate ab and ac
    ab.x = tris[t].b.x - tris[t].a.x; ab.y = tris[t].b.y - tris[t].a.y; ab.z = tris[t].b.z - tris[t].a.z;
    ac.x = tris[t].c.x - tris[t].a.x; ac.y = tris[t].c.y - tris[t].a.y; ac.z = tris[t].c.z - tris[t].a.z;

    // Calculate ab x ac
    x = ab.y*ac.z - ab.z*ac.y;
    y = ab.z*ac.x - ab.x*ac.z;
    z = ab.x*ac.y - ab.y*ac.x;

    // Area is 1/2 | ab x ac |
    area += sqrt(x*x + y*y + z*z)/2.0;
  }
  
  return area;
}

////////////////////////////////////// MyApp ///////////////////////////////////////

IMPLEMENT_APP(MyApp)

void MyApp::microsecond_time(unsigned long long &t)
// Returns the current time in microseconds
{
  struct timeval tv;

  gettimeofday(&tv, NULL);
  t = (unsigned long long)tv.tv_usec + 1000000 * (unsigned long long)tv.tv_sec;
}

void MyApp::OnIdle(wxIdleEvent &event)
// Callback for when the application is idle - is registered and unregistered in MyFrame::OnFly
{
  static unsigned long long last_time = 0;
  unsigned long long this_time;
  static int fps_update_counter = 0;
  float time_diff;

  // Update frames-per-second display
  microsecond_time(this_time);
  if (last_time == 0) last_time = this_time;
  else {
    fps_update_counter++;
    time_diff = (this_time - last_time)/1.0E6;
    if (time_diff >= FPS_UPDATE_INTERVAL) {
      frame->fps = (int)(fps_update_counter / time_diff);
      frame->SetStatusText(wxString::Format("%d frames per second", frame->fps), 1);
      last_time = this_time;
      fps_update_counter = 0;
    }
  }

  // Redraw the top left canvas, the Update() ensures that the frame rate
  // is correct, as long as "Sync to VBlank" is not set in nvidia-settings.
  frame->tl_canvas->Refresh();
  frame->tl_canvas->Update();
}

bool MyApp::OnInit()
// This function is called automatically when the application starts
{
  // Check we have one command line argument
  if (argc != 2) {
    wcout << "Usage:      " << argv[0] << " [filename]" << endl;
    exit(1);
  }

  // Construct the GUI - just fits on a CentOS 1280x1024 workstation
  frame = new MyFrame(NULL, "Laboratory 3G4", wxString(argv[1]), wxDefaultPosition, wxSize(1280, 1020));
  frame->SetMinSize(wxSize(400,400));
  frame->Show(true);
  SetTopWindow(frame);
  return(true); // enter the GUI event loop
}
