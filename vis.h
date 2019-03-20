#ifndef vis_h
#define vis_h

#include <sys/time.h>
#include <wx/wx.h>
#include <wx/glcanvas.h>
#include "isosurf.h"

// Constants
#define PART_PIXEL 0.1
#define FPS_UPDATE_INTERVAL 1.0
#define VOLUME_WIDTH 350
#define VOLUME_HEIGHT 440
#define NUM_SLICES 145
#define SLICE_SEPARATION 2.439
#define DELTA_Z 2.439
#define PIXEL_DIMENSION 0.0005
#define VOLUME_DEPTH ((int) ((NUM_SLICES - 1) * SLICE_SEPARATION) + 1)
#define ORTHO_VIEW_DEPTH 4000
#define PERSPECTIVE_VIEW_NEAR 10
#define PERSPECTIVE_VIEW_FAR 10000
#define INITIAL_DEPTH_OFFSET ORTHO_VIEW_DEPTH/2
#define FLY_ANGLE_STEP 5.0
#define FLY_SPEED_STEP 4.0
const int MAX_TRAVERSAL = ceil(sqrt(VOLUME_WIDTH*VOLUME_WIDTH + VOLUME_HEIGHT*VOLUME_HEIGHT + VOLUME_DEPTH*VOLUME_DEPTH));

// Widget identifiers
enum { 
  VA_SLIDER_ID = wxID_HIGHEST + 1,
  RO_SLIDER_ID,
  SO_SLIDER_ID,
  ST_SLIDER_ID,
  SR_SLIDER_ID,
  XY_SLIDER_ID,
  XZ_SLIDER_ID,
  YZ_SLIDER_ID,
  THETA_ID,
  PHI_ID,
  PSI_ID,
  ISO_BUTTON_ID,
  SURFACE_CHECKBOX_ID,
  LIGHTS_CHECKBOX_ID,
  FLY_CHECKBOX_ID,
  SLICE_CHECKBOX_ID,
  INTERP_CHECKBOX_ID
};

// Canvas identifiers
enum canvas_t { CANVAS_TL = 0, CANVAS_TR = 1, CANVAS_BL = 2, CANVAS_BR = 4};

// The wxWidgets OpenGL drawing canvas class
class MyGLCanvas: public wxGLCanvas
{
 public:
  MyGLCanvas(wxWindow *parent, wxWindowID id = wxID_ANY, canvas_t type = CANVAS_TL, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, 
	     long style = 0, const wxString& name = "MyGLCanvas", const wxPalette &palette=wxNullPalette);
  wxGLContext *context;
  bool configured, wireframe;
  float offset_x, offset_z, velocity_x, velocity_z, heading;
 private:
  canvas_t canvas_type;
  float scene_scale, scene_pan[2], scene_rotate[16];
  void ConfigureGL();
  void OnSize(wxSizeEvent& event);
  void OnPaint(wxPaintEvent& event);
  void OnMouse(wxMouseEvent& event);
  void OnKey(wxKeyEvent& event);
  void draw_view_window(void);
  void draw_surface_in_view(void);
  void draw_reslice_in_view(bool texture_valid, GLuint texture, float image_width, float image_height, int tex_width, int tex_height, float colour[], float coords[]);
  DECLARE_EVENT_TABLE()
};

// The wxWidgets main frame class
class MyFrame: public wxFrame
{
 public:
  MyFrame(wxWindow *parent, const wxString& title, wxString filename, const wxPoint& pos, const wxSize& size, long style = wxDEFAULT_FRAME_STYLE);
  unsigned char data_rgb[VOLUME_WIDTH * VOLUME_HEIGHT * 3], xz_rgb[VOLUME_WIDTH * VOLUME_DEPTH * 3], yz_rgb[VOLUME_HEIGHT * VOLUME_DEPTH * 3], any_rgb[MAX_TRAVERSAL * MAX_TRAVERSAL * 12];
  unsigned char data[VOLUME_WIDTH * VOLUME_HEIGHT * NUM_SLICES], MapItoR[256], MapItoG[256], MapItoB[256];
  MyGLCanvas *tl_canvas;
  int view_angle, data_tex_width, data_tex_height, xz_tex_width, xz_tex_height, yz_tex_width, yz_tex_height, any_tex_height, any_tex_width, surface_alpha, reslice_alpha, fps;
  float data_slice_number, x_reslice_position, y_reslice_position, theta, phi, psi;
  GLuint data_texture, xz_texture, yz_texture, any_texture, tri_list;
  bool data_texture_valid, xz_texture_valid, yz_texture_valid, any_texture_valid, fly, mobile_lights, show_surface, show_reslices;
  void construct_data_slice(void);
  void construct_xz_reslice(void);
  void construct_yz_reslice(void);
  void construct_any_reslice(void);
 private:
  unsigned char threshold;
  triangle *tris;
  int surface_resolution, n_tris;
  bool interpolate;
  MyGLCanvas *tr_canvas, *bl_canvas, *br_canvas;
  void OnClose(wxCloseEvent& event);
  void OnQuit(wxCommandEvent& event);
  void OnAbout(wxCommandEvent& event);
  void OnVASlider(wxScrollEvent &event);
  void OnROSlider(wxScrollEvent &event);
  void OnSOSlider(wxScrollEvent &event);
  void OnSRSlider(wxScrollEvent &event);
  void OnSTSlider(wxScrollEvent &event);
  void OnXYSlider(wxScrollEvent &event);
  void OnXZSlider(wxScrollEvent &event);
  void OnYZSlider(wxScrollEvent &event);
  void OnTHETASlider(wxScrollEvent &event);
  void OnPHISlider(wxScrollEvent &event);
  void OnPSISlider(wxScrollEvent &event);
  void OnSlice(wxCommandEvent &event);
  void OnSurface(wxCommandEvent &event);
  void OnLights(wxCommandEvent &event);
  void OnFly(wxCommandEvent &event);
  void OnCalcIso(wxCommandEvent &event);
  void OnInterp(wxCommandEvent &event);
  void construct_xz_reslice_nearest_neighbour(void);
  void construct_yz_reslice_nearest_neighbour(void);
  void construct_any_reslice_nearest_neighbour(void);
  double mesh_area(void);
  double mesh_volume(void);
  void set_threshold_colourmap(void);
  bool generate_texture(int image_width, int image_height, int &tex_width, int &tex_height, GLuint texture, unsigned char *image);
  DECLARE_EVENT_TABLE()
};

// The wxWidgets application class
class MyApp: public wxApp
{
 public:
   virtual void OnIdle(wxIdleEvent& event);
   bool OnInit();
 private:
   void microsecond_time(unsigned long long &t);
};
    
#endif
