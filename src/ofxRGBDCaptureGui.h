
#pragma once

#include "ofMain.h"
#include "ofxMSAInteractiveObjectDelegate.h"
#include "ofxGameCamera.h"
#include "ofxTimeline.h"
#include "ofxTLDepthImageSequence.h"
#include "ofxDepthImageCompressor.h"
#include "ofxDepthImageProvider.h"
#include "ofxDepthImageRecorder.h"
#include "ofxCvCheckerboardPreview.h"
#include "ofxDepthImageProvider.h"
#include "ofxRGBDScene.h"
#include "ofxRGBDGPURenderer.h"
#include "ofxRGBDCPURenderer.h"

typedef enum {
	TabIntrinsics,
	TabExtrinsics,
	TabCapture,
	TabPlayback
} RecorderTab;

typedef enum {
	RenderBW,
	RenderRainbow,
	RenderPointCloud
} DepthRenderMode;

typedef struct {
	ofxRGBDScene* sceneRef;
	ofxMSAInteractiveObjectWithDelegate* button;
	bool isSelected;
} SceneButton;

typedef struct{
	ofRectangle depthImageRect;
	ofImage depthImage;
	ofShortPixels depthPixelsRaw;
	
	ofRectangle depthCheckersRect;
	ofImage depthCheckers;
	
	ofRectangle colorCheckersRect;
	ofImage colorCheckers;
	
	ofRectangle includeRect;
	ofRectangle deleteRect;
	bool included;
	
} AlignmentPair;

class ofxRGBDCaptureGui : public ofxMSAInteractiveObjectDelegate {
public:
	ofxRGBDCaptureGui();
	~ofxRGBDCaptureGui();

	void setup();
	void setImageProvider(ofxDepthImageProvider* imageProvider);
	void update(ofEventArgs& args);
  	void draw(ofEventArgs& args);

	void mousePressed(ofMouseEventArgs& args);
	void mouseMoved(ofMouseEventArgs& args);
	void mouseDragged(ofMouseEventArgs& args);
	void mouseReleased(ofMouseEventArgs& args);
	
	void keyPressed(ofKeyEventArgs& args);
	void keyReleased(ofKeyEventArgs& args);
	
	void objectDidRollOver(ofxMSAInteractiveObject* object, int x, int y);
	void objectDidRollOut(ofxMSAInteractiveObject* object, int x, int y);
	void objectDidPress(ofxMSAInteractiveObject* object, int x, int y, int button);
	void objectDidRelease(ofxMSAInteractiveObject* object, int x, int y, int button);
	void objectDidMouseMove(ofxMSAInteractiveObject* object, int x, int y);
	
	void dragEvent(ofDragInfo& dragInfo);
	
	void exit(ofEventArgs& args);
	
  protected:
	ofPtr<ofxDepthImageProvider> depthImageProvider;
	ofxTimeline timeline;
	ofxTLDepthImageSequence depthSequence;
	ofxDepthImageRecorder recorder;
	ofxCvCheckerboardPreview calibrationPreview;
   	ofxGameCamera cam;
	ofxGameCamera pointcloudPreviewCam;
	
	ofxRGBDRenderer* renderer;
	ofxRGBDCPURenderer cpuRenderer;
	ofxRGBDGPURenderer gpuRenderer;
	
	ofxRGBDCPURenderer pointcloudPreview;
	
	ofTrueTypeFont contextHelpTextLarge;
	ofTrueTypeFont contextHelpTextSmall;
	ofRectangle hoverRect;
	
	bool providerSet;

	string workingDirectory;
	string rgbCalibrationDirectory;
	string depthCalibrationDirectory;
	string correspondenceDirectory;
	string matrixDirectory;
	
	RecorderTab currentTab;
	DepthRenderMode currentRenderMode;

	ofRectangle previewRectLeft;
	ofRectangle previewRectRight;
 
	ofxMSAInteractiveObjectWithDelegate* currentTabObject;
	ofxMSAInteractiveObjectWithDelegate* currentRenderModeObject;

	vector<ofxMSAInteractiveObjectWithDelegate*> buttonSet; //all non scene buttons
	
	//TOP LEVEL FOLDER
	ofxMSAInteractiveObjectWithDelegate* btnSetDirectory;
	
	//TABS
	ofxMSAInteractiveObjectWithDelegate* btnIntrinsicsTab;
	ofxMSAInteractiveObjectWithDelegate* btnExtrinsicsTab;
	ofxMSAInteractiveObjectWithDelegate* btnRecordTab;
	ofxMSAInteractiveObjectWithDelegate* btnPlaybackTab;

	//RENDERING
	ofxMSAInteractiveObjectWithDelegate* btnRenderBW;
	ofxMSAInteractiveObjectWithDelegate* btnRenderRainbow;
	ofxMSAInteractiveObjectWithDelegate* btnRenderPointCloud;

	//INTRINSICS
	ofxMSAInteractiveObjectWithDelegate* btnRGBLoadCalibration;
	ofxMSAInteractiveObjectWithDelegate* btnCalibrateDepthCamera;
	
	//EXTRINSICS
	ofxMSAInteractiveObjectWithDelegate* btnGenerateCalibration;
	
	//CAPTURE
	ofxMSAInteractiveObjectWithDelegate* btnToggleRecord;

	vector<ofxMSAInteractiveObjectWithDelegate*> tabSet;
	vector<SceneButton> btnScenes;
	
	//main drawing functions
	void drawIntrinsics();
	void drawExtrinsics();
	void drawCapture();
	void drawPlayback();
	void drawSceneButtons();
	void drawCalibrationNumbers();
	void drawDimensionsEntry();
	void drawDepthImage(ofRectangle& targetRect);
	
	void loadDirectory();
	void loadDirectory(string path);
	void loadDefaultDirectory();
	
	bool loadSequenceForPlayback( int index );
	void updateInterfaceForTab(ofxMSAInteractiveObject* tab);
	void updateSceneButtons();
	void disableSceneButtons();
	void enableSceneButtons();
	
	ofColor confirmedColor;
	ofColor warningColor;
	ofColor errorColor;
	
	ofColor downColor;
	ofColor idleColor;
	ofColor hoverColor;
	
	float framewidth;
	float frameheight;
	float thirdWidth;
	float btnheight;
	float sceneWidth;
	float margin;

	ofSoundPlayer recordOn;
	ofSoundPlayer recordOff;
	
	bool backpackMode;
	
	//Preview
	void updateDepthImage(ofShortPixels& pixels);
	ofImage depthImage;
	void createRainbowPallet();
	unsigned char LUTR[256];
	unsigned char LUTG[256];
	unsigned char LUTB[256];
	
	//INTRINSICS
	void loadRGBIntrinsicImages();
	void loadRGBIntrinsicImages(string filepath);
	void loadRGBIntrinsicImages(vector<string> filepaths);
	void saveRGBIntrinsicImages();

	void squareSizeChanged(string& args);
	
	bool addRGBImageToIntrinsicSet(ofImage& image, string fileName);
	vector<ofImage> rgbCalibrationImages;
	vector<string> rgbCalibrationFileNames;

	Calibration rgbCalibration;
	Calibration depthCalibrationBase;
	Calibration depthCalibrationRefined;

	//depth camera params
	ofVec2f fov;
	ofVec2f pp;
	
	int currentCalibrationImageIndex;
	void refineDepthCalibration();
	bool depthCameraSelfCalibrated;
	
	//EXTRINSICS
	vector<AlignmentPair*> alignmentPairs;
	AlignmentPair* currentAlignmentPair;
	void clearCorrespondenceImages();	
	void saveCorrespondenceImages();
	void saveCorrespondenceIncludes();
	void generateCorrespondence();
	
	bool hasIncludedBoards;
	void previewNextAlignmentPair();
	void previewPreviousAlignmentPair();

	string squareSizeFilePath;
	ofxTextInputField checkerboardDimensions;
	
	ofImage hoverPreviewImage;
	bool hoverPreviewDepth;
	bool hoverPreviewIR;
	bool hoverPreviewingCaptured;
	ofImage previewPixelsUndistorted;
	ofVec3f depthToWorldFromCalibration(int x, int y, unsigned short z);
	float squareSize;
	vector< ofColor > boardColors;
	ofMesh inlierPoints;
	
	//CALIBRATION PREVIEW
	void setupRenderer();
	bool calibrationGenerated;
	int currentRendererPreviewIndex;

	
	//RECORDING
	void toggleRecord();

	
	
};