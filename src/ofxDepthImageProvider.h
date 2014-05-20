/*
 *  ofxRGBDepthFrameProvider.h
 *  ofxRGBDepthCaptureOpenNI
 *
 *  Created by Jim on 3/13/12.
 *
 */

#pragma once

#include "ofMain.h"

class ofxDepthImageProvider {
  public:
	ofxDepthImageProvider();
	
	virtual void setup(int deviceId = 0, bool useColor = false) = 0;
	virtual void update() = 0;
	virtual int maxDepth() = 0;	
	virtual void close() = 0;
	virtual ofVec3f getWorldCoordinateAt(int x, int y) = 0;
	
	bool isFrameNew();
	bool deviceFound();

	ofShortPixels& getRawDepth();
	ofImage& getRawIRImage();
	ofImage& getColorImage();
	
	void setDepthModeRainbow(bool useRainbow); //otherwise grayscale
	
  protected:
	bool bDepthImageDirty;
	bool bUseRainbow;
	
	bool bDeviceFound;
	bool bNewFrame;
	
	ofShortPixels depthPixels;
	ofImage rawIRImage;
	ofImage depthImage;
	ofImage colorImage;
};