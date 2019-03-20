struct point {
  double x;
  double y;
  double z;
};

struct triangle {
  point a;
  point b;
  point c;
};

triangle *isosurf(int width, int height, int num_slices, float resolution, float slice_sep, unsigned char *data, int threshold, GLuint trilist, int &n_tris);
