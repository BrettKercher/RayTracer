//
// GraphicalUI.cpp
//
// Handles FLTK integration and other user interface tasks
//
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

#ifndef COMMAND_LINE_ONLY

#include <FL/fl_ask.H>
#include "debuggingView.h"

#include "GraphicalUI.h"
#include "../RayTracer.h"

#define MAX_INTERVAL 500

#ifdef _WIN32
#define print sprintf_s
#else
#define print sprintf
#endif

bool GraphicalUI::stopTrace = false;
bool GraphicalUI::doneTrace = true;
GraphicalUI* GraphicalUI::pUI = NULL;
char* GraphicalUI::traceWindowLabel = "Raytraced Image";
bool TraceUI::m_debug = false;
bool TraceUI::m_antiAlias = false;
bool TraceUI::m_sShade = false;
int GraphicalUI::num_threads = 8;
bool GraphicalUI::done = false;
bool TraceUI::m_kdTree = true;
bool TraceUI::m_cubeMap = false;

//------------------------------------- Help Functions --------------------------------------------
GraphicalUI* GraphicalUI::whoami(Fl_Menu_* o)	// from menu item back to UI itself
{
	return ((GraphicalUI*)(o->parent()->user_data()));
}

//--------------------------------- Callback Functions --------------------------------------------
void GraphicalUI::cb_load_scene(Fl_Menu_* o, void* v) 
{
	pUI = whoami(o);

	static char* lastFile = 0;
	char* newfile = fl_file_chooser("Open Scene?", "*.ray", NULL );

	if (newfile != NULL) {
		char buf[256];

		if (pUI->raytracer->loadScene(newfile)) {
			print(buf, "Ray <%s>", newfile);
			stopTracing();	// terminate the previous rendering
		} else print(buf, "Ray <Not Loaded>");

		pUI->m_mainWindow->label(buf);
		pUI->m_debuggingWindow->m_debuggingView->setDirty();

		if( lastFile != 0 && strcmp(newfile, lastFile) != 0 )
			pUI->m_debuggingWindow->m_debuggingView->resetCamera();

		pUI->m_debuggingWindow->redraw();
	}
}

void GraphicalUI::cb_load_cubemap(Fl_Menu_* o, void* v) 
{
	pUI = whoami(o);
	pUI->m_cubeMapChooser->setCaller(pUI);
	pUI->m_cubeMapChooser->show();
}

void GraphicalUI::cb_save_image(Fl_Menu_* o, void* v) 
{
	pUI = whoami(o);

	char* savefile = fl_file_chooser("Save Image?", "*.bmp", "save.bmp" );
	if (savefile != NULL) {
		pUI->m_traceGlWindow->saveImage(savefile);
	}
}

void GraphicalUI::cb_exit(Fl_Menu_* o, void* v)
{
	pUI = whoami(o);

	// terminate the rendering
	stopTracing();

	pUI->m_traceGlWindow->hide();
	pUI->m_mainWindow->hide();
	pUI->m_debuggingWindow->hide();
	TraceUI::m_debug = false;
}

void GraphicalUI::cb_exit2(Fl_Widget* o, void* v) 
{
	pUI = (GraphicalUI *)(o->user_data());

	// terminate the rendering
	stopTracing();

	pUI->m_traceGlWindow->hide();
	pUI->m_mainWindow->hide();
	pUI->m_debuggingWindow->hide();
	TraceUI::m_debug = false;
}

void GraphicalUI::cb_about(Fl_Menu_* o, void* v) 
{
	fl_message("RayTracer Project for CS384g.");
}

void GraphicalUI::cb_sizeSlides(Fl_Widget* o, void* v)
{
	pUI=(GraphicalUI*)(o->user_data());

	// terminate the rendering so we don't get crashes
	stopTracing();

	pUI->m_nSize=int(((Fl_Slider *)o)->value());
	int width = (int)(pUI->getSize());
	int height = (int)(width / pUI->raytracer->aspectRatio() + 0.5);
	pUI->m_traceGlWindow->resizeWindow(width, height);
}

void GraphicalUI::cb_depthSlides(Fl_Widget* o, void* v)
{
	((GraphicalUI*)(o->user_data()))->m_nDepth=int( ((Fl_Slider *)o)->value() ) ;
}

void GraphicalUI::cb_refreshSlides(Fl_Widget* o, void* v)
{
	((GraphicalUI*)(o->user_data()))->refreshInterval=clock_t(((Fl_Slider *)o)->value()) ;
}

void GraphicalUI::cb_numThreads(Fl_Widget* o, void* v)
{
	((GraphicalUI*)(o->user_data()))->num_threads=int(((Fl_Slider *)o)->value()) ;
}

void GraphicalUI::cb_debuggingDisplayCheckButton(Fl_Widget* o, void* v)
{
	pUI=(GraphicalUI*)(o->user_data());
	pUI->m_displayDebuggingInfo = (((Fl_Check_Button*)o)->value() == 1);
	if (pUI->m_displayDebuggingInfo)
	  {
	    pUI->m_debuggingWindow->show();
	    pUI->m_debug = true;
	  }
	else
	  {
	    pUI->m_debuggingWindow->hide();
	    pUI->m_debug = false;
	  }
}

void GraphicalUI::cb_cmCheckButton(Fl_Widget* o, void* v)
{
	pUI=(GraphicalUI*)(o->user_data());
	pUI->m_usingCubeMap = (((Fl_Check_Button*)o)->value() == 1);
	if (pUI->m_usingCubeMap)
	  {
	    pUI->m_cubeMap = true;
	  }
	else
	  {
	    pUI->m_cubeMap = false;
	  }
}

void GraphicalUI::cb_ssCheckButton(Fl_Widget* o, void* v)
{
	pUI=(GraphicalUI*)(o->user_data());
	pUI->m_smoothshade = (((Fl_Check_Button*)o)->value() == 1);
	if (pUI->m_smoothshade)
	  {
	    pUI->m_sShade = true;
	  }
	else
	  {
	    pUI->m_sShade = false;
	  }
}

void GraphicalUI::cb_treeCheckButton(Fl_Widget* o, void* v)
{
	pUI=(GraphicalUI*)(o->user_data());
	pUI->m_useTree = (((Fl_Check_Button*)o)->value() == 1);
	if (pUI->m_useTree)
	  {
	    pUI->m_kdTree = true;
	  }
	else
	  {
	    pUI->m_kdTree = false;
	  }
}

void GraphicalUI::cb_antialiasCheckButton(Fl_Widget* o, void* v)
{
	pUI=(GraphicalUI*)(o->user_data());
	pUI->m_antiAliasChecked = (((Fl_Check_Button*)o)->value() == 1);
	if (pUI->m_antiAliasChecked)
	  {
	    pUI->m_antiAlias = true;
	  }
	else
	  {
	    pUI->m_antiAlias = false;
	  }
}

void GraphicalUI::cb_treeDepth(Fl_Widget* o, void* v)
{
	((GraphicalUI*)(o->user_data()))->m_treeDepth=int(((Fl_Slider *)o)->value()) ;
}

void GraphicalUI::cb_treeLeaves(Fl_Widget* o, void* v)
{
	((GraphicalUI*)(o->user_data()))->m_treeLeaves=int(((Fl_Slider *)o)->value()) ;
}

void GraphicalUI::cb_antialiasPixels(Fl_Widget* o, void* v)
{
	((GraphicalUI*)(o->user_data()))->m_aaPixels=int(((Fl_Slider *)o)->value()) ;
}

void GraphicalUI::cb_antialiasThresh(Fl_Widget* o, void* v)
{
	((GraphicalUI*)(o->user_data()))->m_aaThresh=int(((Fl_Slider *)o)->value()) ;
}

void GraphicalUI::check_progress(int total)
{
	clock_t now, prev;
	now = prev = clock();
	clock_t intervalMS = pUI->refreshInterval * 100;

	while(!done)
	{
		now = clock();
		if ((now - prev)/CLOCKS_PER_SEC * 1000 >= intervalMS)
		{
			prev = now;
			pUI->m_traceGlWindow->refresh();
			Fl::check();
		 	if (Fl::damage()) Fl::flush();

			std::this_thread::yield();
		}
	}
	pUI->m_traceGlWindow->refresh();
	Fl::check();
 	if (Fl::damage()) Fl::flush();
}

void GraphicalUI::cb_render(Fl_Widget* o, void* v) {

	char buffer[256];
	std::thread t[num_threads];
	std::thread progressThread;
	int startingPixels[num_threads];
	done = false;


	pUI = (GraphicalUI*)(o->user_data());
	doneTrace = stopTrace = false;
	if (pUI->raytracer->sceneLoaded())
	  {
		int width = pUI->getSize();
		int height = (int)(width / pUI->raytracer->aspectRatio() + 0.5);
		int origPixels = width * height;
		pUI->m_traceGlWindow->resizeWindow(width, height);
		pUI->m_traceGlWindow->show();
		pUI->raytracer->traceSetup(width, height);

		int pixelsPerThread = origPixels / num_threads;

		//std::cout << "Per Thread: " << pixelsPerThread << std::endl;

		for(int i = 0; i < num_threads; i++)
		{
			startingPixels[i] = pixelsPerThread * i;
		}

		for (int i = 0; i < num_threads; ++i) 
		{
			t[i] = std::thread(thread_render, startingPixels[i], pixelsPerThread);
		}

		progressThread = std::thread(check_progress, origPixels);

		for (int i = 0; i < num_threads; ++i) 
		{
			t[i].join();
		}

		done = true;
		progressThread.join();

		std::cout << "done" << std::endl;


		// // Save the window label
		const char *old_label = pUI->m_traceGlWindow->label();

		doneTrace = true;
		stopTrace = false;
		// Restore the window label
		pUI->m_traceGlWindow->label(old_label);
		pUI->m_traceGlWindow->refresh();
	  }
}


void GraphicalUI::thread_render(int startingPixel, int pixelsPerThread)
{
	char buffer[256];
	int width = pUI->getSize();
	int height = (int)(width / pUI->raytracer->aspectRatio() + 0.5);

	int x, y, curPixel;

	for(int i = 0; i < pixelsPerThread; i++)
	{
		curPixel = startingPixel + i;
		y = floor(curPixel / height);
		x = curPixel - (width * y);

		pUI->raytracer->tracePixel(x, y);
	}
}

void GraphicalUI::cb_stop(Fl_Widget* o, void* v)
{
	pUI = (GraphicalUI*)(o->user_data());
	stopTracing();
}

int GraphicalUI::run()
{
	Fl::visual(FL_DOUBLE|FL_INDEX);

	m_mainWindow->show();

	return Fl::run();
}

void GraphicalUI::alert( const string& msg )
{
	fl_alert( "%s", msg.c_str() );
}

void GraphicalUI::setRayTracer(RayTracer *tracer)
{
	TraceUI::setRayTracer(tracer);
	m_traceGlWindow->setRayTracer(tracer);
	m_debuggingWindow->m_debuggingView->setRayTracer(tracer);
}

// menu definition
Fl_Menu_Item GraphicalUI::menuitems[] = {
	{ "&File", 0, 0, 0, FL_SUBMENU },
	{ "&Load Scene...",	FL_ALT + 'l', (Fl_Callback *)GraphicalUI::cb_load_scene },
	{ "&Load Cubemap...",	FL_ALT + 'l', (Fl_Callback *)GraphicalUI::cb_load_cubemap },
	{ "&Save Image...", FL_ALT + 's', (Fl_Callback *)GraphicalUI::cb_save_image },
	{ "&Exit", FL_ALT + 'e', (Fl_Callback *)GraphicalUI::cb_exit },
	{ 0 },

	{ "&Help",		0, 0, 0, FL_SUBMENU },
	{ "&About",	FL_ALT + 'a', (Fl_Callback *)GraphicalUI::cb_about },
	{ 0 },

	{ 0 }
};

void GraphicalUI::stopTracing()
{
	stopTrace = true;
}

GraphicalUI::GraphicalUI() : refreshInterval(10) {

	m_aaPixels = 3;
	m_aaThresh = 100;
	m_treeDepth = 15;
	m_treeLeaves = 10;
	m_useTree = true;
	m_cubeMapChooser = new CubeMapChooser();

	// init.
	m_mainWindow = new Fl_Window(100, 40, 450, 459, "Ray <Not Loaded>");
	m_mainWindow->user_data((void*)(this));	// record self to be used by static callback functions
	// install menu bar
	m_menubar = new Fl_Menu_Bar(0, 0, 440, 25);
	m_menubar->menu(menuitems);

	// set up "render" button
	m_renderButton = new Fl_Button(360, 37, 70, 25, "&Render");
	m_renderButton->user_data((void*)(this));
	m_renderButton->callback(cb_render);

	// set up "stop" button
	m_stopButton = new Fl_Button(360, 65, 70, 25, "&Stop");
	m_stopButton->user_data((void*)(this));
	m_stopButton->callback(cb_stop);

	// install depth slider
	m_depthSlider = new Fl_Value_Slider(10, 40, 180, 20, "Recursion Depth");
	m_depthSlider->user_data((void*)(this));	// record self to be used by static callback functions
	m_depthSlider->type(FL_HOR_NICE_SLIDER);
	m_depthSlider->labelfont(FL_COURIER);
	m_depthSlider->labelsize(12);
	m_depthSlider->minimum(0);
	m_depthSlider->maximum(10);
	m_depthSlider->step(1);
	m_depthSlider->value(m_nDepth);
	m_depthSlider->align(FL_ALIGN_RIGHT);
	m_depthSlider->callback(cb_depthSlides);

	// install size slider
	m_sizeSlider = new Fl_Value_Slider(10, 65, 180, 20, "Screen Size");
	m_sizeSlider->user_data((void*)(this));	// record self to be used by static callback functions
	m_sizeSlider->type(FL_HOR_NICE_SLIDER);
	m_sizeSlider->labelfont(FL_COURIER);
	m_sizeSlider->labelsize(12);
	m_sizeSlider->minimum(64);
	m_sizeSlider->maximum(1024);
	m_sizeSlider->step(2);
	m_sizeSlider->value(m_nSize);
	m_sizeSlider->align(FL_ALIGN_RIGHT);
	m_sizeSlider->callback(cb_sizeSlides);

	// install refresh interval slider
	m_refreshSlider = new Fl_Value_Slider(10, 90, 180, 20, "Screen Refresh Interval (0.1 sec)");
	m_refreshSlider->user_data((void*)(this));	// record self to be used by static callback functions
	m_refreshSlider->type(FL_HOR_NICE_SLIDER);
	m_refreshSlider->labelfont(FL_COURIER);
	m_refreshSlider->labelsize(12);
	m_refreshSlider->minimum(1);
	m_refreshSlider->maximum(300);
	m_refreshSlider->step(1);
	m_refreshSlider->value(refreshInterval);
	m_refreshSlider->align(FL_ALIGN_RIGHT);
	m_refreshSlider->callback(cb_refreshSlides);

	// install thread slider
	m_threadSlider = new Fl_Value_Slider(10, 115, 180, 20, "Threads");
	m_threadSlider->user_data((void*)(this));	// record self to be used by static callback functions
	m_threadSlider->type(FL_HOR_NICE_SLIDER);
	m_threadSlider->labelfont(FL_COURIER);
	m_threadSlider->labelsize(12);
	m_threadSlider->minimum(1);
	m_threadSlider->maximum(32);
	m_threadSlider->step(1);
	m_threadSlider->value(num_threads);
	m_threadSlider->align(FL_ALIGN_RIGHT);
	m_threadSlider->callback(cb_numThreads);

	// set up anti aliasing checkbox
	m_antialiasCheckButton = new Fl_Check_Button(10, 160, 180, 20, "Antialiasing");
	m_antialiasCheckButton->user_data((void*)(this));
	m_antialiasCheckButton->callback(cb_antialiasCheckButton);
	m_antialiasCheckButton->value(m_antiAliasChecked);

	// install anti aliasing sliders
	m_aaSamplesSlider = new Fl_Value_Slider(115, 145, 180, 20, "Pixel Samples");
	m_aaSamplesSlider->user_data((void*)(this));	// record self to be used by static callback functions
	m_aaSamplesSlider->type(FL_HOR_NICE_SLIDER);
	m_aaSamplesSlider->labelfont(FL_COURIER);
	m_aaSamplesSlider->labelsize(12);
	m_aaSamplesSlider->minimum(1);
	m_aaSamplesSlider->maximum(4);
	m_aaSamplesSlider->step(1);
	m_aaSamplesSlider->value(m_aaPixels);
	m_aaSamplesSlider->align(FL_ALIGN_RIGHT);
	m_aaSamplesSlider->callback(cb_antialiasPixels);

	m_aaThreshSlider = new Fl_Value_Slider(115, 175, 180, 20, "Supersample Threshold");
	m_aaThreshSlider->user_data((void*)(this));	// record self to be used by static callback functions
	m_aaThreshSlider->type(FL_HOR_NICE_SLIDER);
	m_aaThreshSlider->labelfont(FL_COURIER);
	m_aaThreshSlider->labelsize(12);
	m_aaThreshSlider->minimum(1);
	m_aaThreshSlider->maximum(1000);
	m_aaThreshSlider->step(1);
	m_aaThreshSlider->value(m_aaThresh);
	m_aaThreshSlider->align(FL_ALIGN_RIGHT);
	m_aaThreshSlider->callback(cb_antialiasThresh);

	// set up kd checkbox
	m_kdCheckButton = new Fl_Check_Button(10, 230, 180, 20, "K-d Tree");
	m_kdCheckButton->user_data((void*)(this));
	m_kdCheckButton->callback(cb_treeCheckButton);
	m_kdCheckButton->value(m_useTree);	

	// install kd tree sliders
	m_treeDepthSlider = new Fl_Value_Slider(115, 215, 180, 20, "Max Depth");
	m_treeDepthSlider->user_data((void*)(this));	// record self to be used by static callback functions
	m_treeDepthSlider->type(FL_HOR_NICE_SLIDER);
	m_treeDepthSlider->labelfont(FL_COURIER);
	m_treeDepthSlider->labelsize(12);
	m_treeDepthSlider->minimum(1);
	m_treeDepthSlider->maximum(30);
	m_treeDepthSlider->step(1);
	m_treeDepthSlider->value(m_treeDepth);
	m_treeDepthSlider->align(FL_ALIGN_RIGHT);
	m_treeDepthSlider->callback(cb_treeDepth);

	m_leafSizeSlider = new Fl_Value_Slider(115, 245, 180, 20, "Target Leaf Size");
	m_leafSizeSlider->user_data((void*)(this));	// record self to be used by static callback functions
	m_leafSizeSlider->type(FL_HOR_NICE_SLIDER);
	m_leafSizeSlider->labelfont(FL_COURIER);
	m_leafSizeSlider->labelsize(12);
	m_leafSizeSlider->minimum(1);
	m_leafSizeSlider->maximum(100);
	m_leafSizeSlider->step(1);
	m_leafSizeSlider->value(m_treeLeaves);
	m_leafSizeSlider->align(FL_ALIGN_RIGHT);
	m_leafSizeSlider->callback(cb_treeLeaves);

	// set up smooth shading checkbox
	m_cubeMapCheckButton = new Fl_Check_Button(10, 369, 140, 20, "Cube Mapping");
	m_cubeMapCheckButton->user_data((void*)(this));
	m_cubeMapCheckButton->callback(cb_cmCheckButton);
	m_cubeMapCheckButton->value(m_usingCubeMap);

		// set up smooth shading checkbox
	m_ssCheckButton = new Fl_Check_Button(10, 399, 140, 20, "Smooth Shading");
	m_ssCheckButton->user_data((void*)(this));
	m_ssCheckButton->callback(cb_ssCheckButton);
	m_ssCheckButton->value(m_smoothshade);

	// set up debugging display checkbox
	m_debuggingDisplayCheckButton = new Fl_Check_Button(10, 429, 140, 20, "Debugging display");
	m_debuggingDisplayCheckButton->user_data((void*)(this));
	m_debuggingDisplayCheckButton->callback(cb_debuggingDisplayCheckButton);
	m_debuggingDisplayCheckButton->value(m_displayDebuggingInfo);

	m_mainWindow->callback(cb_exit2);
	m_mainWindow->when(FL_HIDE);
	m_mainWindow->end();

	// image view
	m_traceGlWindow = new TraceGLWindow(100, 150, m_nSize, m_nSize, traceWindowLabel);
	m_traceGlWindow->end();
	m_traceGlWindow->resizable(m_traceGlWindow);

	// debugging view
	m_debuggingWindow = new DebuggingWindow();
}

#endif
