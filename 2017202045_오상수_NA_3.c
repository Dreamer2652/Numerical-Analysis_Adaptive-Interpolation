#include <stdio.h>
#include <malloc.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#define width 256
#define height 256
#define w_origin 512
#define h_origin 512
#define SIZE height*width
#define GROUP 4000      // # of group
#define th 10
#define FILTER_SIZE 3  // All filters 1 axis length
#define weight 20
const int c = FILTER_SIZE>>1;  // Center
const char name[] = "Lena";

unsigned char input[height][width], output[h_origin][w_origin], origin[h_origin][w_origin];

// 4 direction values
int count[GROUP], group_idx[height][width], V[height][width], H[height][width], D_135[height][width], D_45[height][width];
// 3*3 modified laplacian filter
const int filter_V[FILTER_SIZE][FILTER_SIZE] =
{
  {-1,0,-1},
  {-(weight-4>>1),weight,-(weight-4>>1)},
  {-1,0,-1},
},
filter_H[FILTER_SIZE][FILTER_SIZE] =
{
  {-1,-(weight-4>>1),-1},
  {0,weight,0},
  {-1,-(weight-4>>1),-1},
},
filter_45[FILTER_SIZE][FILTER_SIZE] =
{
  {-(weight-4>>1),-1,0},
  {-1,weight,-1},
  {0,-1,-(weight-4>>1)},
},
filter_135[FILTER_SIZE][FILTER_SIZE] =
{
  {0,-1,-(weight-4>>1)},
  {-1,weight,-1},
  {-(weight-4>>1),-1,0},
};

inline int absolute(int x) {return x<0 ? -x : x;}
inline int M(int a, int b) {return a>b ? a : b;}
inline void bilinear(int i, int j)
{
  output[i][j+1] = output[i][j]+output[i][j+2]>>1;
  output[i+1][j] = output[i][j]+output[i+2][j]>>1;
  output[i+1][j+2] = output[i][j+2]+output[i+2][j+2]>>1;
  output[i+2][j+1] = output[i+2][j]+output[i+2][j+2]>>1;
  output[i+1][j+1] = output[i][j+1]+output[i+1][j]+output[i+1][j+2]+output[i+2][j+1]>>2;
}

int main()
{
  char filename[50];
  int i, j, h, w, p, max = 0, min = 1<<30, *group[GROUP];
  FILE* img;
  
  // Open downscaled img
  strcpy(filename, name);
  strcat(filename, "_256x256_yuv400_8bit.raw");
  img = fopen(filename,"rb");
  if(!img)
  {
    printf("FILE OPEN ERROR\n");
    return -1;
  }
  fread(input, sizeof(char), SIZE, img);
  fclose(img);

  // Open original img
  strcpy(filename, name);
  strcat(filename, "_512x512_yuv400_8bit_original.raw");
  img = fopen(filename,"rb");
  if(!img)
  {
    printf("FILE OPEN ERROR\n");
    return -1;
  }
  fread(origin, sizeof(char), sizeof(origin), img);
  fclose(img);
  
  // Integer-pel matching
  for(i = 0; i < height; i++)
    for(j = 0; j < width; j++)
      output[i*2+1][j*2+1] = input[i][j];

  // Padding first
  for(i = 1; i < h_origin-2; i+=2)  // Inside of integer-pel (bilinear)
  {
    for(j = 1; j < FILTER_SIZE; j+=2)
      bilinear(i, j);
    if(i < FILTER_SIZE || i >= h_origin-FILTER_SIZE)
      for(; j < w_origin-FILTER_SIZE; j+=2)
        bilinear(i, j);
    for(j = w_origin-FILTER_SIZE; j < w_origin-2; j+=2)
      bilinear(i, j);
  }
  for(output[0][0] = output[1][1], i = 1; i < h_origin; i++)  // Outline (nearest neighbor)
  {
    output[0][i] = output[1][i];
    output[i][0] = output[i][1];
  }

  // Calculate intensity(abs) of edges(filter)
  for(i = c; i+c < height; i++)
    for(j = c; j+c < width; j++)
    {
      // Apply filter to pixels
      for(h = 0; h < FILTER_SIZE; h++)
        for(w = 0; w < FILTER_SIZE; w++)
        {
          p = input[i+h-c][j+w-c];
          V[i][j] += p*filter_V[h][w];
          H[i][j] += p*filter_H[h][w];
          D_45[i][j] += p*filter_45[h][w];
          D_135[i][j] += p*filter_135[h][w];
        }

        // Apply abs, check max & min
        V[i][j] = absolute(V[i][j]);
        H[i][j] = absolute(H[i][j]);
        D_45[i][j] = absolute(D_45[i][j]);
        D_135[i][j] = absolute(D_135[i][j]);
        max = max < V[i][j]+H[i][j] ? V[i][j]+H[i][j] : max;
        min = min > V[i][j]+H[i][j] ? V[i][j]+H[i][j] : min;
      }

  // Pixel Classification
  int x = GROUP/5;
  for(i = c, p = (max-min+x)/x; i+c < height; i++)
    for(j = c; j+c < width; j++)
    {
      // Activity interval : Same([1 1 ... 1 1]*(max-min)/N)
      group_idx[i][j] = (V[i][j]+H[i][j]-min)/p;
      
      // Gradient decision : Maximum intensity
      int grad_max = M(M(V[i][j], H[i][j]), M(D_135[i][j], D_45[i][j]));
      if(grad_max >= th)
      {
        if(grad_max == H[i][j])
          group_idx[i][j] += x;
        else if(grad_max == V[i][j])
          group_idx[i][j] += x*3;
        else if(grad_max == D_45[i][j])
          group_idx[i][j] += x*2;
        else
          group_idx[i][j] += x*4;
      }
      count[group_idx[i][j]]++;  // Count check
    }

  // Pixel grouping
  for(i = 0; i < GROUP; count[i++] = 0)
    if(count[i])
      group[i] = (int*)malloc(count[i]*sizeof(int));  // Dynamic memory allocation
  // Insert pixel
  for(i = c; i+c < height; i++)
    for(j = c; j+c < width; j++)
    {
      p = group_idx[i][j];
      group[p][count[p]++] = i*width+j;
    }
  
  // Adaptive filter - shape(# of group, half-pel(H,V,D), FILTER)
  unsigned long long adaptive[GROUP][3][FILTER_SIZE][FILTER_SIZE] = {0,}, acm[GROUP] = {0,}, sum;
  
  // Least-square filter optimization
  for(int g = 0; g < GROUP; g++)
    for(p = 0; p < count[g]; p++)
    {
      // Get target pixel's idx
      unsigned int target_x = group[g][p]&255, target_y = group[g][p]>>8, origin_pixel, X_pixel;
      
      for(i = 0; i < FILTER_SIZE; i++)
        for(j = 0; j < FILTER_SIZE; j++)
        {
          X_pixel = input[target_y+i-c][target_x+j-c];
          acm[g] += X_pixel*X_pixel;  // Auto-correlation matrix (1*1 scalar)
          
          // Construct H,V,D interpolation filter
          for(int half = 1; half < 4; half++)
          {
            origin_pixel = origin[target_y*2+1-((half&2)>0)][target_x*2+1-((half&1)>0)];
            adaptive[g][half-1][i][j] += X_pixel*origin_pixel;  // Cross-correaltion matrix (1*1 near pixels)
          }
        }
    }
  
  // Interpolation
  for(int g = 0; g < GROUP; g++)
    for(p = 0; p < count[g]; p++)
      // Interpolation H,V,D half-pel
      for(int target_x = group[g][p]&255, target_y = group[g][p]>>8, half = 1; half < 4; half++)
      {
        sum = acm[g]>>1;
        
        for(i = 0; i < FILTER_SIZE; i++)
          for(j = 0; j < FILTER_SIZE; j++)
            sum += adaptive[g][half-1][i][j]*input[target_y+i-c][target_x+j-c];

        sum /= acm[g];
        output[target_y*2+1-((half&2)>0)][target_x*2+1-((half&1)>0)] = sum > 255 ? 255 : sum;
      }
      
  // Free dynamic allocated memory
  for(i = 0; i < GROUP; i++)
    if(count[i])
      free(group[i]);
  
  // Visualize upsampled img
  strcpy(filename, name);
  strcat(filename, "_512x512_yuv400_output.raw");
  img = fopen(filename,"wb");
  fwrite(output, sizeof(char), sizeof(output), img);
  fclose(img);

  // Evaluate RMSE & PSNR
  sum = 0;
  for(i = 0; i < h_origin; i++)
    for(j = 0; j < w_origin; j++)
      sum += (origin[i][j]-output[i][j])*(origin[i][j]-output[i][j]);
  double RMSE = sqrt(1.0*sum/(h_origin*w_origin));
  printf("%s Adaptive Interpolation Result\nMSE = %lf, PSNR = %lf\n", name, RMSE, log10(255/RMSE)*20);
  return 0;
}