#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL.h>
#include <time.h>

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480
#define SCREEN_BPP     24

#define TRUE 1
#define FALSE 0

/* shape colors*/
#define COLOR_BG   0.12f, 0.14f, 0.13f
#define COLOR_TEXT 0.94f, 0.87f, 0.69f
#define COLOR_L  { 0.50f, 0.62f, 0.50f }
#define COLOR_J  { 0.37f, 0.70f, 0.54f }
#define COLOR_I  { 0.94f, 0.88f, 0.67f }
#define COLOR_Z  { 0.55f, 0.82f, 0.83f }
#define COLOR_S  { 0.58f, 0.75f, 0.95f }
#define COLOR_T  { 0.86f, 0.64f, 0.64f }
#define COLOR_O  { 0.44f, 0.31f, 0.31f }

/*  OpenGL and SDL stuff   */
SDL_Surface *surface;
int videoFlags;

/* global game variables */
double gx;
double gy;
unsigned int fi;
unsigned int speed;
unsigned int ticks;
unsigned int score;

/* basic game structures */
struct vec2
{
	double x;
	double y;
};

struct rgb
{
	float r;
	float g;
	float b;
};

struct figure
{
	struct vec2 points[2][4];
	struct rgb color;
	struct vec2 size;
	int state;
}cur_figure;

struct font
{
	GLuint texture;
	GLuint list_base;
}game_font;

struct list
{
	struct vec2 pos;
	struct rgb color;
	struct list* next;
}*l_root;

/* proto */
void font_destroy(struct font*);

/* blocks */
struct figure figures[7]  = {
	/*      figure points       |   turned figure points   |      color   */
	{ { {{1,2},{2,2},{3,2},{3,1}}, {{2,1},{2,2},{2,3},{3,3}} }, COLOR_L, {3,3}, 0 }, /* L */
	{ { {{1,1},{1,2},{2,2},{3,2}}, {{3,1},{2,1},{2,2},{2,3}} }, COLOR_J, {3,3}, 0 }, /* J */
	{ { {{1,2},{2,2},{3,2},{4,2}}, {{2,1},{2,2},{2,3},{2,4}} }, COLOR_I, {4,3}, 0 }, /* I */
	{ { {{1,1},{2,1},{2,2},{3,2}}, {{2,1},{2,2},{1,2},{1,3}} }, COLOR_Z, {3,2}, 0 }, /* Z */
	{ { {{1,2},{2,2},{2,1},{3,1}}, {{1,1},{1,2},{2,2},{2,3}} }, COLOR_S, {3,2}, 0 }, /* S */
	{ { {{1,2},{2,2},{3,2},{2,3}}, {{1,2},{2,1},{2,2},{2,3}} }, COLOR_T, {3,3}, 0 }, /* T */
	{ { {{1,1},{2,1},{1,2},{2,2}}, {{1,1},{2,1},{1,2},{2,2}} }, COLOR_O, {2,2}, 0 }  /* O */
};

/* lists stuff */
struct list* l_find(double x,double y)
{
  struct list* iter = l_root;

  while(iter->pos.x != x && iter->pos.y != y && iter)
    iter = iter->next;

  return iter;
}

unsigned int l_length(void)
{
	struct list* iter = l_root;
	int i=0;

	for(i=0; iter; i++)
		iter = iter->next;

	return i;
}

struct list* l_get(unsigned int n)
{
    int i =0;
    struct list* iter = l_root;

    while(i++ != n && iter)
      iter = iter->next;

    return iter;
}

struct list* l_del(double x,double y)
{
	struct list* it = l_root;
	while(it)
	{
		if(it->next)
		{
			if((it->next->pos.x == x) && (it->next->pos.y == y))
			{
				struct list* to_del = it->next;
				it->next = it->next->next;
				free(to_del);
				break;
			}
		}

		it = it->next;
	}

	if(l_root && (l_root->pos.x == x) && (l_root->pos.y == y))
	{
		struct list* tmp = l_root->next;
		free(l_root);
		l_root = tmp;
		return l_root;
	}

	return it;
}
/*
struct list* l_del(n)
{
	double x1;
	double y1;
	if(l_get(n))
	{
		x1 = l_get(n)->x;
		y1 = l_get(n)->y;
	}
	printf("d %f,%f\n",x1,y1);
	struct list* prev;
	struct list* to_del;

	if(n == 0)
	{
		if((to_del = l_get(n)))
		{
			l_root = to_del->next;
			free(to_del);
		}
		else
			return NULL;
	}
	else
	{
		prev = l_get(n-1);
		to_del = prev->next;

		if(to_del)
		{
			prev->next = to_del->next;
			free(to_del);
		}
		else
			return NULL;
	}

	return prev;
}
*/

struct list* l_append(double x,double y,struct rgb new_color)
{
  struct list* iter = l_root;

  if(iter != NULL)
  {
    while(iter->next)
      iter=iter->next;
  }

  struct list* new = (struct list*) malloc(sizeof(struct list));
  struct vec2 new_pos = {x,y};


  new->pos = new_pos;
  new->next = NULL;
  new->color = new_color;

  if(iter !=NULL)
    iter->next = new;
  else
    l_root = new;

  return new;
}

unsigned int l_count(double x,double y)
{
	struct list* iter = l_root;

	unsigned int counter =0;

	while(iter)
	{
		if(x && y)
			if((iter->pos.x == x) && (iter->pos.y == y))
				counter++;

		if(x && !y)
			if(iter->pos.x == x)
				counter++;

		if(!x && y)
			if(iter->pos.y == y)
				counter++;

		iter = iter->next;
	}

	return counter;
}

/* end of list stuff */

void quit(int ret_code)
{
	font_destroy(&game_font);
	printf("%s\n","Good bye!");
    SDL_Quit();
    exit(ret_code);
}


struct vec2 local2global(double x,double y)
{
	struct vec2 global_coord = { x-5.0f, (20.0f-y)+1.0 - 10.0f };
	return global_coord;
}

void fig_flip(struct figure* f)
{
	int width;
	int height;
	int i;

	if(f->state == 0)
	{
		width = f->size.x;
		height = f->size.y;
	}
	else
	{
		width = f->size.y;
		height = f->size.x;
	}


	for(i=0;i<4;i++)
	{
		f->points[f->state][i].x = width - f->points[f->state][i].x + 1;
		f->points[f->state][i].y = height - f->points[f->state][i].y + 1;
	}
}
struct vec2 fig_size(unsigned int n)
{
	struct vec2 size;

	size.x = cur_figure.points[cur_figure.state][0].x;

	int i;
	for(i=1;i<4;i++)
		if(cur_figure.points[cur_figure.state][i].x > size.x)
			size.x = cur_figure.points[cur_figure.state][i].x;

	size.y = cur_figure.points[cur_figure.state][0].y;

	for(i=1;i<4;i++)
		if(cur_figure.points[cur_figure.state][i].y > size.y)
			size.y = cur_figure.points[cur_figure.state][i].y;

	return size;
}

int check_fill(void)
{
	int j;
	int i;

	for(i=1;i<=20;i++)
	{
		if(l_count(0,i) >= 10)
		{
			for(j=1;j<=10;j++)
				l_del(j,i);

			struct list* iter = l_root;
			while (iter)
			{
				if(iter->pos.y < i)
					iter->pos.y++;

				iter = iter->next;
			}

			score+=(1000/speed)*10;
		}
	}

	return 1;
}

int fig_check_collision_turn(int fig_x,int fig_y)
{
	struct figure f = cur_figure;
	fig_flip(&f);

	int i;
	struct list* iter = l_root;
	while(iter)
	{
		for(i=0;i<4;i++)
		{
			if((iter->pos.x == f.points[f.state][i].x+fig_x &&
				iter->pos.y == f.points[f.state][i].y+fig_y) ||
				f.points[f.state][i].x+fig_x < 1 ||
			    f.points[f.state][i].x+fig_x > 10 ||
			    f.points[f.state][i].y + fig_y > 20)
				return 1;
		}

		iter = iter->next;
	}

	return 0;
}

int fig_check_collision_x(int figx,int figy)
{
	struct list* it;
	int i;
	int s = cur_figure.state;

	for(i=0;i<4;i++)
	{
		if(cur_figure.points[s][i].x+figx+1 > 10)
			return 1;
		if(cur_figure.points[s][i].x+figx-1 < 1)
			return 2;

		it = l_root;

		while(it)
		{
			if(cur_figure.points[s][i].y+figy == it->pos.y)
			{
				if(cur_figure.points[s][i].x+figx+1 == it->pos.x)
						return 1;
				if(cur_figure.points[s][i].x+figx-1 == it->pos.x)
						return 2;
			}
			it = it->next;
		}
	}

	return 0;
}

int fig_check_collision_y(int figx,int figy)
{
	struct list* it;
	int i;
	int s = cur_figure.state;

	for(i=0;i<4;i++)
	{
		it = l_root;

		while(it)
		{
			if(cur_figure.points[s][i].x+figx == it->pos.x && cur_figure.points[s][i].y+figy+1 == it->pos.y)
				return 1;

			it = it->next;

		}
	}

	return 0;
}
/*
double get_highest(void)
{
	struct list* it = l_root->next;
	double max = l_root->y;

	while(it)
	{
		if(it->y > max)
			max = it->y;

		it = it->next;
	}

	return max;
}
*/

void fig_fallen(void)
{
	int s = cur_figure.state;
	int i;
	for(i=0;i<4;i++)
		l_append(cur_figure.points[s][i].x+gx,cur_figure.points[s][i].y+gy,cur_figure.color);
}

int win_resize(int new_width,int new_height)
{
    GLfloat ratio;

    if (new_height == 0)
	new_height = 1;

    ratio = (GLfloat) new_width / (GLfloat) new_height;

    glViewport(0, 0, (GLsizei) new_width,(GLsizei) new_height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(45.0f, ratio, 0.1f, 100.0f);

    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();

    return TRUE;
}

unsigned int on_timer(unsigned int interval,void* param)
{

	return interval;
}

void on_collision(void)
{
	if( (gy == (20 - fig_size(fi).y)) || (fig_check_collision_y(gx,gy) == 1))
	{
		fig_fallen();

		if(gy<1)
		{
			printf("Loser!\n");
			quit(0);
		}


		if(score % 100 == 0 && score>0 && speed > 50)
			speed -= 50;

		fi = rand() % 7;
		cur_figure = figures[fi];

		gy=-3;
		gx= 5 - cur_figure.size.x;
	}
}

void on_key(SDL_keysym *keysym)
{
    switch (keysym->sym)
	{
	case SDLK_ESCAPE:
	    quit(0);
	    break;
	case SDLK_F1:
	    SDL_WM_ToggleFullScreen(surface);
	    break;
	case SDLK_LEFT:
		if(fig_check_collision_x(gx,gy)!=2)
			gx--;
		break;
		on_collision();
	case SDLK_RIGHT:
		if(fig_check_collision_x(gx,gy)!=1)
			gx++;
		on_collision();
		break;
	case SDLK_UP:
		cur_figure.state = !cur_figure.state;

		if(fig_check_collision_turn(gx,gy) == 0)
			fig_flip(&cur_figure);
		else
			cur_figure.state = !cur_figure.state;

		on_collision();
		break;
	case SDLK_DOWN:
		while((fig_check_collision_y(gx,gy) == 0) && gy < (20-fig_size(fi).y))
			gy++;

		on_collision();
		break;
	case SDLK_q:
		if(fi<6)
		{
			fi++;
			cur_figure = figures[fi];
		}
		break;
	case SDLK_e:
		if(fi>0)
		{
			fi--;
			cur_figure = figures[fi];
		}
		break;
	default:
	    break;
	}
}

int font_load(char* file,struct font* ft_out)
{
	FILE* fp;

	unsigned short int bfType;
    long int bfOffBits;
    short int biPlanes;
    short int biBitCount;
    long int biSizeImage;

	struct texture
	{
		int height;
		int width;
		unsigned char* data;
	}font_texture;

	if ((fp = fopen(file, "rb")) == NULL)
        return -1;

    if(!fread(&bfType, sizeof(short int), 1, fp))
        return -1;

    if (bfType != 19778)
        return -1;

	fseek(fp, 8, SEEK_CUR);

	fread(&bfOffBits, sizeof(long int), 1, fp);

	fseek(fp,4,SEEK_CUR);

	fread(&font_texture.width, sizeof(int), 1, fp);
    fread(&font_texture.height, sizeof(int), 1, fp);

	fread(&biPlanes, sizeof(short int), 1, fp);
    if (biPlanes != 1)
        return -1;

    fread(&biBitCount, sizeof(short int), 1, fp);

    if (biBitCount != 24)
		return -1;

    biSizeImage = font_texture.width * font_texture.height * 3;
    font_texture.data = (unsigned char*) malloc(biSizeImage);

	fseek(fp, bfOffBits, SEEK_SET);

	fread(font_texture.data, biSizeImage, 1, fp);

	/* bgr -> rgb */
	unsigned char temp;
	int i;
	for (i = 0; i < biSizeImage; i += 3)
    {
        temp = font_texture.data[i];
        font_texture.data[i] = font_texture.data[i + 2];
        font_texture.data[i + 2] = temp;
    }

	glGenTextures(1,&ft_out->texture);
	glBindTexture(GL_TEXTURE_2D,ft_out->texture);

	glTexImage2D(GL_TEXTURE_2D, 0, 3, font_texture.width,
				 font_texture.height, 0, GL_RGB, GL_UNSIGNED_BYTE,
				 font_texture.data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	ft_out->list_base = glGenLists(256);

	float cx,cy;
	int loop;
	for (loop = 0; loop < 256; loop++)
    {
        cx = (float) (loop % 16) / 16.0f;
        cy = (float) (loop / 16) / 16.0f;

		glNewList(ft_out->list_base + loop, GL_COMPILE);
		 glBegin(GL_QUADS);
		  glTexCoord2f(cx, 1 - cy - 0.0625f);
		  glVertex3f(0.0f, 0.0f, 0.0f);

		  glTexCoord2f(cx + 0.0625f, 1 - cy - 0.0625f);
		  glVertex3f(0.5,0.0,0.0);

		  glTexCoord2f(cx + 0.0625f, 1 - cy);
		  glVertex3f(0.5f, 0.5f, 0.0f);

		  glTexCoord2f(cx, 1 - cy);
		  glVertex3f(0.0f, 0.5f, 0.0f);
		 glEnd();

		 glTranslatef(0.4f, 0.0f, 0.0f);
        glEndList();
    }

	free(font_texture.data);
	fclose(fp);

    return 0;
}

void font_destroy(struct font* ft)
{
	glDeleteLists(ft->list_base,256);
	glDeleteTextures(1,&ft->texture);
}


void font_render(struct font* ft,GLfloat x, GLfloat y, char *string, int set)
{
	glBindTexture(GL_TEXTURE_2D, ft->texture);
	glPushMatrix();
	glTranslatef(x,y,0.0f);
	glListBase(ft->list_base +(128*set));
	glCallLists(strlen(string),GL_BYTE,string);
	glPopMatrix();
}

int init_gfx(GLvoid)
{
    const SDL_VideoInfo *videoInfo;

    if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
		return -1;

    videoInfo = SDL_GetVideoInfo( );

    if ( !videoInfo )
		return -1;

    /* the flags to pass to SDL_SetVideoMode */
    videoFlags  = SDL_OPENGL;          /* Enable OpenGL in SDL */
    videoFlags |= SDL_GL_DOUBLEBUFFER; /* Enable double buffering */
    videoFlags |= SDL_HWPALETTE;       /* Store the palette in hardware */
    videoFlags |= SDL_RESIZABLE;       /* Enable window resizing */

    /* This checks to see if surfaces can be stored in memory */
    if ( videoInfo->hw_available )
		videoFlags |= SDL_HWSURFACE;
    else
		videoFlags |= SDL_SWSURFACE;

    /* This checks if hardware blits can be done */
    if ( videoInfo->blit_hw )
		videoFlags |= SDL_HWACCEL;

    /* Sets up OpenGL double buffering */
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    /* get a SDL surface */
    surface = SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP,
				videoFlags );

    /* Verify there is a surface */
    if ( !surface )
		return -1;

	/* setup OpenGL */
	glShadeModel(GL_SMOOTH);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glEnable(GL_TEXTURE_2D);


    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	/* set window size */
    win_resize ( SCREEN_WIDTH, SCREEN_HEIGHT );

	glFlush();

    return 0;
}

void draw_block(double x,double y,struct rgb color)
{
	struct vec2 glob_coord = local2global(x,y);

	glColor3f(color.r,color.g,color.b);
    glBegin( GL_QUADS );                /* Draw A Quad */
      glVertex3f(  glob_coord.x-1.0f, glob_coord.y,      0.0f ); /* Top Left */
      glVertex3f(  glob_coord.x,      glob_coord.y,      0.0f ); /* Top Right */
      glVertex3f(  glob_coord.x,      glob_coord.y-1.0f, 0.0f ); /* Bottom Right */
      glVertex3f(  glob_coord.x-1.0f, glob_coord.y-1.0f, 0.0f ); /* Bottom Left */
    glEnd( );                           /* Done Drawing The Quad */
}

void draw_figure(double fig_x,double fig_y)
{
	int i;
	for(i=0;i<4;i++)
		draw_block(cur_figure.points[cur_figure.state][i].x+fig_x,
				   cur_figure.points[cur_figure.state][i].y+fig_y,
				   cur_figure.color);
}

int render(GLvoid)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -25.0f);

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	/* draw background */
	glColor3f(0.1f,0.1f,0.1f);
    glBegin( GL_QUADS );
      glVertex3f( -5.0f,  10.0f, 0.0f );
      glVertex3f(  5.0f,  10.0f, 0.0f );
      glVertex3f(  5.0f, -10.0f, 0.0f );
      glVertex3f( -5.0f, -10.0f, 0.0f );
    glEnd( );

	/* draw blocks */
	struct list* iter = l_root;;
	while(iter) {
		draw_block(iter->pos.x,iter->pos.y,iter->color);
		iter = iter->next;
	}

	/* draw figure */
	draw_figure(gx,gy);

	/* draw grid */

	glColor3f(0.0f,0.0f,0.0f);

	double i,j;
	for(i=-5.0;i<5.0;i++)
	{
		glBegin(GL_LINES);
			glVertex3f(i,10.0,0.0f);
			glVertex3f(i,-10.0,0.0f);
		glEnd();
	}
	for(j=-10.0;j<10.0;j++)
	{
		glBegin(GL_LINES);
			glVertex3f(-5.0,j,0.0f);
			glVertex3f(5.0,j,0.0f);
		glEnd();
	}

	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	//glLoadIdentity();

	/* draw scores and level */
	glColor3f(COLOR_TEXT);
	char tmp[128];
	snprintf(tmp,120,"Level: %i",score / 100);
	font_render(&game_font,6.0,2.0,tmp,0);
	snprintf(tmp,120,"Score: %i",score);
	font_render(&game_font,6.0,1.0,tmp,0);

    /* Draw it to the screen */
    SDL_GL_SwapBuffers( );

    return TRUE;
}

/* TODO: parameters:
 * -l <n>      set level
 * -f          fullscreen mode
 * -s <w>x<h>  set window size
*/
int main( int argc, char **argv )
{
    int done = FALSE;
    int isActive = TRUE;

	srand(time(NULL));

	l_root = NULL;
	gx = 4.0f;
	gy = 0.0f;
	speed = 1000;
	ticks = 0;
	score = 0;
	fi = rand() % 7;
	cur_figure = figures[fi];

	printf("%s","Video initialization...");

	if( init_gfx() < 0 )
	{
		printf("%s\n","FAIL: Graphics init failed");
		quit(1);
	}
	if( font_load("data/font.bmp",&game_font) < 0 )
	{
		printf("%s\n","FAIL: Can not load font");
		quit(1);
	}

	SDL_WM_SetCaption("Zenburn Tetris",NULL);

	printf("%s","OK\n");

	/*
	l_append(9,9);
	l_append(1,1);
	l_append(2,2);
	l_append(3,3);

	l_del(3);
	int i=0;
	for(i=0;i<3;i++)
		printf("%f\n",l_get(i)->x);

	printf("getting length...\n");
	printf("len %i\n",l_length());
	*/

    SDL_Event event;

    while ( !done )
	{
	    /* handle the events in the queue */

	    while ( SDL_PollEvent( &event ) )
		{
		    switch( event.type )
			{
			case SDL_ACTIVEEVENT:
			    /* Something's happend with our focus
			     * If we lost focus or we are iconified, we
			     * shouldn't draw the screen
			     */
			    if ( event.active.gain == 0 )
				isActive = FALSE;
			    else
				isActive = TRUE;
			    break;
			case SDL_VIDEORESIZE:
			    /* handle resize event */
			    surface = SDL_SetVideoMode( event.resize.w,
							event.resize.h,
							16, videoFlags );
			    if ( !surface )
				{
				    fprintf( stderr, "Could not get a surface after resize: %s\n", SDL_GetError( ) );
				    quit( 1 );
				}
			    win_resize( event.resize.w, event.resize.h );
			    break;
			case SDL_KEYDOWN:
			    /* handle key presses */
			    on_key( &event.key.keysym );
			    break;
			case SDL_QUIT:
			    /* handle quit requests */
			    done = TRUE;
			    break;
			default:
			    break;
			}
		}

		if( (SDL_GetTicks() - ticks) >= speed )
		{
			gy++;
			ticks = SDL_GetTicks();

			check_fill();

			/* collision check */
			on_collision();
		}


	    /* draw the scene */
	    if ( isActive )
			render( );
	}

    /* clean ourselves up and exit */
    quit( 0 );

    /* Should never get here */
    return( 0 );
}
