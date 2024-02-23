#include "rasterizer.h"

using namespace std;

namespace CGL {

  RasterizerImp::RasterizerImp(PixelSampleMethod psm, LevelSampleMethod lsm,
    size_t width, size_t height,
    unsigned int sample_rate) {
    this->psm = psm;
    this->lsm = lsm;
    this->width = width;
    this->height = height;
    this->sample_rate = sample_rate;

    sample_buffer.resize(width * height * sample_rate, Color::White);
  }

  // Used by rasterize_point and rasterize_line
  void RasterizerImp::fill_pixel(size_t x, size_t y, Color c) {
    // TODO: Task 2: You might need to this function to fix points and lines (such as the black rectangle border in test4.svg)
    // NOTE: You are not required to implement proper supersampling for points and lines
    // It is sufficient to use the same color for all supersamples of a pixel for points and lines (not triangles)

    for(int i = 0; i < (int)(sqrt(sample_rate)); i++){
      for(int j = 0; j < (int)(sqrt(sample_rate)); j++){
        sample_buffer[y * width * sample_rate + i * width * (int)(sqrt(sample_rate)) + x * (int)(sqrt(sample_rate)) + j] = c;
      }
    }
  }

  // Rasterize a point: simple example to help you start familiarizing
  // yourself with the starter code.
  //
  void RasterizerImp::rasterize_point(float x, float y, Color color) {
    // fill in the nearest pixel
    int sx = (int)floor(x);
    int sy = (int)floor(y);

    // check bounds
    if (sx < 0 || sx >= width) return;
    if (sy < 0 || sy >= height) return;

    fill_pixel(sx, sy, color);
    return;
  }

  // Rasterize a line.
  void RasterizerImp::rasterize_line(float x0, float y0,
    float x1, float y1,
    Color color) {
    if (x0 > x1) {
      swap(x0, x1); swap(y0, y1);
    }

    float pt[] = { x0,y0 };
    float m = (y1 - y0) / (x1 - x0);
    float dpt[] = { 1,m };
    int steep = abs(m) > 1;
    if (steep) {
      dpt[0] = x1 == x0 ? 0 : 1 / abs(m);
      dpt[1] = x1 == x0 ? (y1 - y0) / abs(y1 - y0) : m / abs(m);
    }

    while (floor(pt[0]) <= floor(x1) && abs(pt[1] - y0) <= abs(y1 - y0)) {
      rasterize_point(pt[0], pt[1], color);
      pt[0] += dpt[0]; pt[1] += dpt[1];
    }
  }


  bool RasterizerImp::is_inside_edge(float x, float y, 
    float x1, float y1,
    float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return (x1 - x) * dy + (y - y1) * dx >= 0;
  }

  // Rasterize a triangle.
  void RasterizerImp::rasterize_triangle(float x0, float y0,
    float x1, float y1,
    float x2, float y2,
    Color color) {
    // TODO: Task 1: Implement basic triangle rasterization here, no supersampling
    float pt[2];
    //find bound
    int min_x = floor(min(min(x0, x1), x2));
    int min_y = floor(min(min(y0, y1), y2));
    int max_x = floor(max(max(x0, x1), x2));
    int max_y = floor(max(max(y0, y1), y2));

    float offset = 1.0 / (sqrt(sample_rate) * 2);
    float min_pt[] = { min_x + offset,min_y + offset };
    float max_pt[] = { max_x + 1.0,max_y + 1.0 };

    if((x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0) < 0){
        swap(x1, x2);
        swap(y1, y2);
    }

    int sx;
    int sy;
    for(pt[0] = min_pt[0], sx = (int)(floor(pt[0]) * sqrt(sample_rate)); pt[0] <= max_pt[0]; pt[0]+=offset * 2, sx++){
      for(pt[1] = min_pt[1], sy = (int)(floor(pt[1]) * sqrt(sample_rate)); pt[1] <= max_pt[1]; pt[1]+=offset * 2, sy++){
        if(is_inside_edge(pt[0], pt[1], x0, y0, x1, y1)
          && is_inside_edge(pt[0], pt[1], x1, y1, x2, y2)
          && is_inside_edge(pt[0], pt[1], x2, y2, x0, y0)){
            // rasterize_point(pt[0], pt[1], color);
            // fill in the nearest pixel
            if (sx < 0 || sx >= width * (int)(sqrt(sample_rate))) return;
            if (sy < 0 || sy >= height * (int)(sqrt(sample_rate))) return;
            sample_buffer[sy * width * (int)(sqrt(sample_rate)) + sx] = color;
          }
      }
    }
    // TODO: Task 2: Update to implement super-sampled rasterization
  }


  void RasterizerImp::rasterize_interpolated_color_triangle(float x0, float y0, Color c0,
    float x1, float y1, Color c1,
    float x2, float y2, Color c2)
  {
    // TODO: Task 4: Rasterize the triangle, calculating barycentric coordinates and using them to interpolate vertex colors across the triangle
    // Hint: You can reuse code from rasterize_triangle
    float pt[2];
    //find bound
    int min_x = floor(min(min(x0, x1), x2));
    int min_y = floor(min(min(y0, y1), y2));
    int max_x = floor(max(max(x0, x1), x2));
    int max_y = floor(max(max(y0, y1), y2));

    float offset = 1.0 / (sqrt(sample_rate) * 2);
    float min_pt[] = { min_x + offset,min_y + offset };
    float max_pt[] = { max_x + 1.0,max_y + 1.0 };

    if((x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0) < 0){
        Color c_reg = c1;
        c1 = c2;
        c2 = c_reg;
        swap(x1, x2);
        swap(y1, y2);
    }

    int sx;
    int sy;
    float b_fac[3];

    for(pt[0] = min_pt[0], sx = (int)(floor(pt[0]) * sqrt(sample_rate)); pt[0] <= max_pt[0]; pt[0]+=offset * 2, sx++){
      for(pt[1] = min_pt[1], sy = (int)(floor(pt[1]) * sqrt(sample_rate)); pt[1] <= max_pt[1]; pt[1]+=offset * 2, sy++){
        if(is_inside_edge(pt[0], pt[1], x0, y0, x1, y1)
          && is_inside_edge(pt[0], pt[1], x1, y1, x2, y2)
          && is_inside_edge(pt[0], pt[1], x2, y2, x0, y0)){
            // rasterize_point(pt[0], pt[1], color);
            // fill in the nearest pixel
            if (sx < 0 || sx >= width * (int)(sqrt(sample_rate))) return;
            if (sy < 0 || sy >= height * (int)(sqrt(sample_rate))) return;
            b_fac[0] = ((x1 - pt[0]) * (y2 - y1) + (pt[1] - y1) * (x2 - x1)) / ((x1 - x0) * (y2 - y1) + (y0 - y1) * (x2 - x1));
            b_fac[1] = ((x2 - pt[0]) * (y0 - y2) + (pt[1] - y2) * (x0 - x2)) / ((x2 - x1) * (y0 - y2) + (y1 - y2) * (x0 - x2));
            b_fac[2] = 1 - b_fac[0] - b_fac[1];
            sample_buffer[sy * width * (int)(sqrt(sample_rate)) + sx] = c0 * b_fac[0] + c1 * b_fac[1] + c2 * b_fac[2];
          }
      }
    }
  }


  void RasterizerImp::rasterize_textured_triangle(float x0, float y0, float u0, float v0,
    float x1, float y1, float u1, float v1,
    float x2, float y2, float u2, float v2,
    Texture& tex)
  {
    // TODO: Task 5: Fill in the SampleParams struct and pass it to the tex.sample function.
    // TODO: Task 6: Set the correct barycentric differentials in the SampleParams struct.
    // Hint: You can reuse code from rasterize_triangle/rasterize_interpolated_color_triangle




  }

  void RasterizerImp::set_sample_rate(unsigned int rate) {
    // TODO: Task 2: You may want to update this function for supersampling support

    this->sample_rate = rate;

    //add: rate
    this->width = width;
    this->height = height;
    this->sample_buffer.resize(width * height * sample_rate, Color::White);
  }


  void RasterizerImp::set_framebuffer_target(unsigned char* rgb_framebuffer,
    size_t width, size_t height)
  {
    // TODO: Task 2: You may want to update this function for supersampling support

    this->width = width;
    this->height = height;
    this->rgb_framebuffer_target = rgb_framebuffer;

    //add: rate
    this->sample_buffer.resize(width * height * sample_rate, Color::White);
  }


  void RasterizerImp::clear_buffers() {
    std::fill(rgb_framebuffer_target, rgb_framebuffer_target + 3 * width * height, 255);
    std::fill(sample_buffer.begin(), sample_buffer.end(), Color::White);
  }


  // This function is called at the end of rasterizing all elements of the
  // SVG file.  If you use a supersample buffer to rasterize SVG elements
  // for antialising, you could use this call to fill the target framebuffer
  // pixels from the supersample buffer data.
  //
  void RasterizerImp::resolve_to_framebuffer() {
    // TODO: Task 2: You will likely want to update this function for supersampling support

    int super_width = width * (int)(sqrt(sample_rate));
    int super_height = height * (int)(sqrt(sample_rate));
    int len = sqrt(sample_rate);
    int k;
    int sx;
    int sy;
    for (int x = 0; x < width; ++x) {
      for (int y = 0; y < height; ++y) {
        //TODO: average color
        sx = x * len;
        sy = y * len;
        Color col = Color::White;
        if(sample_rate == 1) col = sample_buffer[y * width + x];
        else{
          for(int i = 0; i < len; ++i){
            for(int j = 0; j < len; ++j){
              if(i == 0 && j == 0) col = sample_buffer[sy * super_width + sx];
              else col += sample_buffer[sy * super_width + sx + i * (super_width) + j];
            }
          }
          for(k = 0; k < 3; ++k){
            (&col.r)[k] = ((&col.r)[k]) / sample_rate;
          }
        }
        for (k = 0; k < 3; ++k) {
          this->rgb_framebuffer_target[3 * (y * width + x) + k] = (&col.r)[k] * 255;
        }
      }
    }

  }

  Rasterizer::~Rasterizer() { }


}// CGL
