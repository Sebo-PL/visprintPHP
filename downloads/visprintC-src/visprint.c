//////////////////////////////////////////////////////////////////////////
// Visprint -- visual fingerprint generator 2.0
//
// visprint is a program to produce visual fingerprints. This code was
// written by Ian Goldberg, based on an idea by Hal Finney, in a post to
// the coderpunks mailing list. The most excellent color enhancements
// were added by Raph Levien.  David Johnston Ported the program to 
// Windows 95 console mode and added a bunch of nice features.
// Soren Andersen developed the png and transparency code.
// Goizeder Ruiz de Gopegui rescued the code from the oblivion of the
// internet and decided to maintain it.
//
// Ian Golderg:    http://http.cs.berkeley.edu/~iang/
// Raph Leviev:    http://www.cs.berkeley.edu/~raph/
// Hal Finney:     http://www.rain.org/~hal/
// David Johnston: http://www.cybercities.com/ston/
// Soren Andersen:  http://intrepid.perlmonk.org/
// Goizeder Ruiz de Gopegui: http://www.tastyrabbit.net
//
// Last modified 28 Nov 2006
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <png.h>
#include <sys/io.h>

#define MAX_RES            1000
#define DEFAULT_RES        300
#define MAX_CHECKSUM       128    
#define DEFAULT_CHECKSUM   32
#define DEFAULT_INTENSITY  30
#define DEFAULT_BACKGROUND 0  
#define DEFAULT_COLOUR     1  

typedef unsigned char byte;

byte pic[MAX_RES][MAX_RES][3];
png_bytep row_pointers[MAX_RES];

double eqns[MAX_CHECKSUM/8][6];
byte hashd[MAX_CHECKSUM/8][6];

int res, neqns, intensity, background, checksum;
int transparency = 0;
int alternatergb = 0;
char *str_version = "Visprint 2.1 by Goizeder Ruiz de Gopegui (http://www.tastyrabbit.net/visprint)\n";
char *str_usage =
	"Visprint 2.1 (http://www.tastyrabbit.net/visprint)\n"
	"Usage: md5sum(sha1sum) foo.bar | visprint [Options] > foo.png\n"
	"Options:\n\n"
	"-b(0-255)  Use the given background brightness. 0 means black, 255 means white. You can try other values,\n" 
	"           but the resultant image will be quite ugly.The default is 0 (black).\n"
	"\n"
	"-c         Use the alternate coloring method. Flips RGB to BGR.\n"
	"\n"
	"-a         Create the fractal with 4 different color areas.\n"
	"\n"
	"-g         Create a semi-grayscale image. The output varies heavily depending on the -b switch.\n"
	"\n"
	"-i(1-lots) Use the given intensity of color. With a black background, a higher intensity produces a \n"
        "           brighter image. With a white background, higher intensity produces a darker image.\n"
	"           In all cases, calculation time doubles every time you double the intensity. The default is 30.\n"
	"\n"
	"-l(32-128) Use the given checksum length. Modify this number to match the hash function you are using. \n"
	"           Examples: 32 for md5, 40 for sha1, 64 for sha256, 128 for sha512...\n" 
	"           The output of 128 is foggy (but still useful). Higher values will result in crappy images,\n"
	"           therefore 128 is the maximum accepted. The default value is 32.\n"
	"\n"
	"-r(1-1000) Use the given value for both the x and y resolutions. The calculation time will quadruple \n"
        "           every time you double this value. The default is 300.\n"
	"\n"	
	"-t         Create the image with a transparent background.\n"
	"\n"
	"-v         Print version information.\n"
	"\n"
	"-?,-h      Print this help.\n"
	"\n";

//////////////////////////////////////////////////////////////////////////

void inithashd(void)
{
    byte buf[8];
    int i;
    int j;

    for(i=0;i<neqns;++i) 
    {

      fread(buf, 8, 1, stdin);
      for(j=0;j<8;++j) 
      {
        byte c = buf[j];
        if (c >= '0' && c <= '9')
          buf[j] = c-'0';
        else if (c >= 'a' && c <= 'f')
          buf[j] = c-('a'-10);
        else if (c >= 'A' && c <= 'F')
          buf[j] = c-('A'-10);
        else buf[j] = 0;
      }
      hashd[i][0] = ( buf[0] << 2 ) | ( buf[1] >> 2 );
      hashd[i][1] = ( (buf[1]&3) << 4 ) | buf[2];
      hashd[i][2] = ( buf[3] << 2 ) | ( buf[4] >> 2 );
      hashd[i][3] = ( (buf[4]&3) << 4 ) | buf[5];
      hashd[i][4] = buf[6];
      hashd[i][5] = buf[7];
    }

}

//////////////////////////////////////////////////////////////////////////

void initeqn(int n, double s, double t, double a, double b, double u,
    double v)
{
    eqns[n][0] = s*(1+a*b);
    eqns[n][1] = s*b;
    eqns[n][2] = t*a;
    eqns[n][3] = t;
    eqns[n][4] = u;
    eqns[n][5] = v;
}

//////////////////////////////////////////////////////////////////////////

void initeqns(void)
{
    int i;
    double s,t,a,b,u,v;

    for(i=0;i<neqns;++i) {
	s = (hashd[i][0])*0.3/64.0 + 0.3;
	t = (hashd[i][1])*0.3/64.0 + 0.3;
	a = (hashd[i][2])/32.0 - 1.0;
	b = (hashd[i][3])/32.0 - 1.0;
	u = (hashd[i][4])/32.0 - 0.25;
	v = (hashd[i][5])/32.0 - 0.25;
	initeqn(i,s,t,a,b,u,v);
    }
}

//////////////////////////////////////////////////////////////////////////

int gfcn = 0;
int colour = 0;

//////////////////////////////////////////////////////////////////////////

void find_window(double *xtrans, double *ytrans, double *scalefactor)
{
  int i, j, k;

  double x = 0, y = 0;
  double nx, ny;

  double xdiff, ydiff;
  double maxx=0, minx=0, maxy=0, miny=0;

  int fcn;

  for(i = 0;i < (50 * res); i++)  // iterate a few times first
  {
    fcn = rand()%neqns; // choose 

    nx = eqns[fcn][0]*x + eqns[fcn][1]*y + eqns[fcn][4];
    ny = eqns[fcn][2]*x + eqns[fcn][3]*y + eqns[fcn][5];
    x=nx;
    y=ny;
  }

  for(; i < (60 *res); i++)  // now continue where we just left off for a small amount of time
  {
    fcn = rand()%neqns; // choose 

    nx = eqns[fcn][0]*x + eqns[fcn][1]*y + eqns[fcn][4];
    ny = eqns[fcn][2]*x + eqns[fcn][3]*y + eqns[fcn][5];

    if (i>res*50)
    {
      if (nx < minx)
        minx = nx;
      if (nx > maxx)
        maxx = nx;
      if (ny < miny)
        miny = ny;
      if (ny > maxy)
        maxy = ny;
    }
    x=nx;
    y=ny;
  }

  xdiff = (maxx - minx) * 1.05;
  ydiff = (maxy - miny) * 1.05;

  if (xdiff > ydiff)
    *scalefactor = 1 / xdiff;
  else *scalefactor = 1 / ydiff;

  *xtrans = ((maxx + minx) / 2);
  *ytrans = ((maxy + miny) / 2);
}


//////////////////////////////////////////////////////////////////////////

void nextpoint(double x, double y, double *nx, double *ny)
{
    int fcn = rand()%neqns; // choose 

    if (colour && (colour>1 || ((rand() & 1) == 0))) gfcn = fcn + 1;
    *nx = eqns[fcn][0]*x + eqns[fcn][1]*y + eqns[fcn][4];
    *ny = eqns[fcn][2]*x + eqns[fcn][3]*y + eqns[fcn][5];
}

//////////////////////////////////////////////////////////////////////////

void ifs(void)
{
    double nx,ny;
    double x = 1;//rand()%res;
    double y = 2;//rand()%res;
    int i,j,k;
    int res2, nxres2, nyres2;
    int max_intensity;
    double xtrans, ytrans;
    double scalefactor;

    find_window(&xtrans, &ytrans, &scalefactor);
    
    max_intensity = 255 - background;

    for(i=0;i<res;++i) for(j=0;j<res;++j) for(k=0;k<3;k++)
      pic[i][j][k]=background;

    for(i = 0;i < (50 * res); i++)  // iterate a few times first
    {
      nextpoint(x,y,&nx,&ny);
	    x=nx;
      y=ny;
    }

    for(;i<intensity*res*res; i++)
    {
      nextpoint(x,y,&nx,&ny);
      nxres2 = ((nx - xtrans) * scalefactor + 0.5) * res;
      nyres2 = ((ny - ytrans) * scalefactor + 0.5) * res;
//      fprintf(stderr, "nxres2: %d  nyres2: %d  scalefactor: %f\n", nxres2, nyres2, scalefactor);
      if (i>res*50 && nxres2>=0 && nxres2<res && nyres2>=0 && nyres2<res)
      {
	      if (colour)
        {
          if(background == 255)
          {
            if ((gfcn & 1) && pic[nxres2][nyres2][0] != max_intensity)
              pic[nxres2][nyres2][0]--;
            if ((gfcn & 2) && pic[nxres2][nyres2][1] != max_intensity)
              pic[nxres2][nyres2][1]--;
            if ((gfcn & 4) && pic[nxres2][nyres2][2] != max_intensity)
              pic[nxres2][nyres2][2]--;
          }
          else
          {
            if ((gfcn & 1) && pic[nxres2][nyres2][0] != max_intensity)
              pic[nxres2][nyres2][0]++;
            if ((gfcn & 2) && pic[nxres2][nyres2][1] != max_intensity)
              pic[nxres2][nyres2][1]++;
            if ((gfcn & 4) && pic[nxres2][nyres2][2] != max_intensity)
              pic[nxres2][nyres2][2]++;
          }
        }
	//2 more lines added to fix the grayscale png creation
        else
        { pic[nxres2][nyres2][0] = max_intensity;
	  pic[nxres2][nyres2][1] = max_intensity;
          pic[nxres2][nyres2][2] = max_intensity;
        }
      }
	    x=nx;
      y=ny;
    }
}

//////////////////////////////////////////////////////////////////////////

void writepnm(void)
{
    int i,j;
    if (colour)
    {
        printf("P6\n%d %d\n255\n",res,res);
        for(i = 0; i < res; i++)
          fwrite(pic[i], 3, res, stdout);
    }
    else
    {
        printf("P6\n%d %d\n255\n",res,res);
        for(j=0;j<res;++j)
        {
          for (i=0;i<res;++i)
          {
            fwrite (pic[i][j], 1, 1, stdout);
            fwrite (pic[i][j], 1, 1, stdout);
            fwrite (pic[i][j], 1, 1, stdout);
          }
        }
    } 
}

//////////////////////////////////////////////////////////////
// void parse_cmdline(int argc,char *argv[])
//
// Added by David Johnston on Aug 12th 1998
// Fixed and modified by Goiz Ruiz 2006
// Parses the command line for switches and other arguments
//////////////////////////////////////////////////////////////

void parse_cmdline(int argc,char *argv[])
{
  int x;

  colour     = DEFAULT_COLOUR;
  checksum   = DEFAULT_CHECKSUM;
  res        = DEFAULT_RES;
  intensity  = DEFAULT_INTENSITY;
  background = DEFAULT_BACKGROUND;

  for(x=1;x<argc;x++)
    {
//      fprintf(stderr,"looking at %s\n",argv[x]);
      
      if(!strcmp("-a",argv[x]))
        colour = 2;
      else if(!strcmp("-c",argv[x]))
        alternatergb = 1;
      else if(!strcmp("-g",argv[x]))
        colour = 0;
      else if(!strcmp("-t",argv[x]))
        transparency = 1;
      else if(!strcmp("-r",argv[x]))
      {
	      x++;
        if(sscanf(argv[x],"%d",&res)==0)
        {
          fprintf(stderr, "invalid resolution on command line \n");
	        exit(1);
        }
      }
      else if(argv[x][0]=='-' && argv[x][1]=='r')
      {
        if (sscanf(argv[x]+2,"%d",&res)==0)
        {
          fprintf(stderr, "invalid resolution on command line \n");
	        exit(1);
        }
      }
      else if(!strcmp("-l",argv[x]))
      {
	      x++;
        if(sscanf(argv[x],"%d",&checksum)==0)
        {
          fprintf(stderr, "invalid checksum length on command line \n");
	        exit(1);
        }
      }
	
      else if(argv[x][0]=='-' && argv[x][1]=='l')
      {
        if (sscanf(argv[x]+2,"%d",&checksum)==0)
        {
          fprintf(stderr, "invalid checksum length on command line");
	        exit(1);
        }
      }
      else if(!strcmp("-i",argv[x]))
      {
	      x++;
        if(sscanf(argv[x],"%d",&intensity)==0)
        {
          fprintf(stderr, "invalid intensity on command line");
	        exit(1);
        }
      }
      else if(argv[x][0]=='-' && argv[x][1]=='i')
      {
        if (sscanf(argv[x]+2,"%d",&intensity)==0)
        {
          fprintf(stderr, "invalid intensity on command line");
	        exit(1);
        }
      }
      else if(!strcmp("-b",argv[x]))
      {
	      x++;
        if(sscanf(argv[x],"%d",&background)==0)
        {
          fprintf(stderr, "invalid background on command line");
	        exit(1);
        }
      }
      else if(argv[x][0]=='-' && argv[x][1]=='b')
      {
        if (sscanf(argv[x]+2,"%d",&background)==0)
        {
          fprintf(stderr, "invalid background on command line");
	        exit(1);
        }
      }
      else if((!strcmp(argv[x], "-?"))||(!strcmp(argv[x], "--help"))||(!strcmp(argv[x], "-h")))
      {
        printf( str_usage );
			exit(0);
      }
      else if(!strcmp(argv[x], "-v"))
      {
        printf( str_version );
			exit(0);
      }
      else
      {
        fprintf(stderr, "Unknown command line switch. Try -? or --help for more information.\n");
        exit(1);
      }        
  } 

  if(res > MAX_RES)
  {
    fprintf(stderr, "% is too high a resolution.  Assuming %d.\n", res, MAX_RES);
    res = MAX_RES;
  }

  if(checksum > MAX_CHECKSUM)
  {
    fprintf(stderr, "%d checksumlength is too big.  Assuming %d.\n", checksum, DEFAULT_CHECKSUM);
    checksum = DEFAULT_CHECKSUM;
  }
 neqns= checksum/8;
}


//////////////////////////////////////////////////////////////////////////
// PNG STUFF

void user_error_fn(png_structp png_ptr, png_const_charp error_msg) {
   fprintf(stderr, "error in PNG: %s\n", error_msg);
   exit(1);
}
void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg) {
   fprintf(stderr, "warning in PNG (but exiting)\n: ", warning_msg);
   exit(1);
}
void write_row_callback(png_structp png_ptr, png_uint_32 row, int pass) {
   if (png_ptr == NULL || row > 0x3fffffffL || pass > 7) return;
   fprintf(stderr, "#");
}

int writepng (void) {
   png_structp png_ptr;
   png_infop info_ptr;
   png_color_16 transvalue;
   unsigned i, j;

   /* Initialisation of PNG structures */
   png_ptr = png_create_write_struct(
      PNG_LIBPNG_VER_STRING, (png_voidp*) 0,
      &user_error_fn, &user_warning_fn);
   if (! png_ptr) {
      fprintf(stderr, "png_create_write_struct() failed\n");
      return -1;
   }
   info_ptr = png_create_info_struct(png_ptr);
   if (! info_ptr) {
      fprintf(stderr, "png_create_info_struct() failed\n");
      png_destroy_write_struct(&png_ptr, NULL);
      return -1;
   }

   png_init_io(png_ptr, stdout);

   /* Some feedback during write if you wish :) */
   /* png_set_write_status_fn(png_ptr, &write_row_callback); */

   /* Set header and write it */
   
  png_set_IHDR(png_ptr, info_ptr, res, res, 8, PNG_COLOR_TYPE_RGB,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                PNG_FILTER_TYPE_DEFAULT);
   

   /* Set #000000 to transparent */
   if (transparency==1){
	//bzero(&background, sizeof(background));
	transvalue.red = background;
        transvalue.green = background;
        transvalue.blue = background;
   	png_set_tRNS(png_ptr, info_ptr, 0, 0, &transvalue);
   }
   
   for (i = 0; i < res; ++i)
      row_pointers[i] = (png_bytep) pic[i];
   png_set_rows(png_ptr, info_ptr, row_pointers);
   
   if (alternatergb==1){
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_BGR, NULL);
   }
   else	{	
   	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
   }

/* Uncomment if you enabled feedback during write above */
   /* fprintf(stderr, "\n"); */
   
   fclose(stdout);
   return 0;
}


//////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    parse_cmdline(argc, argv);

    srand(time(NULL));
    inithashd();
    initeqns();
    ifs();
   // writepnm();
    writepng();
    return 0;
}
