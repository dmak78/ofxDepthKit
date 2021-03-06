/**
 * ofxRGBDepth addon
 *
 * Core addon for programming against the RGBDToolkit API
 * http://www.rgbdtoolkit.com
 *  http://github.com/obviousjim
 *
 * (c) James George 2011-2013 http://www.jamesgeorge.org
 *
 * Developed with support from:
 *	  Frank-Ratchy STUDIO for Creative Inquiry http://studioforcreativeinquiry.org/
 *	  YCAM InterLab http://interlab.ycam.jp/en
 *	  Eyebeam http://eyebeam.org
 */

#include "ofxRGBDGPURenderer.h"
#define GLSL(version, extension, shader)  "#version " #version "\n#extension " #extension "\n" #shader

const GLchar* vert = GLSL(110, GL_ARB_texture_rectangle : enable,
						  
uniform vec2 dim;
uniform vec2 textureScale;

// CORRECTION
uniform vec2 shift;
uniform vec2 scale;

// DEPTH
uniform sampler2DRect depthTex;
uniform vec2 principalPoint;
uniform vec2 fov;
uniform float farClip;
uniform float nearClip;

uniform float edgeClip;

uniform float leftClip;
uniform float rightClip;
uniform float topClip;
uniform float bottomClip;

uniform vec2 simplify;

//COLOR INTRINSICS
uniform mat4 extrinsics;
uniform vec2 colorFOV;
uniform vec2 colorPP;
uniform vec3 dK;
uniform vec2 dP;

varying float positionValid;

const float epsilon = 1e-6;

float depthAtPosition(vec2 samplePosition){
  
  if(samplePosition.x >= 640.0*leftClip &&
	 samplePosition.x <= 640.0*rightClip &&
	 samplePosition.y >= 480.0*topClip &&
	 samplePosition.y <= 480.0*bottomClip)
  {
	  return texture2DRect(depthTex, samplePosition).r * 65535.;
  }
  return 0.0;
}

///MAIN ---------------------------
void main(void)
{
    //align to texture
    vec2 halfvec = vec2(.5,.5);
	
    float depth = depthAtPosition(floor(gl_Vertex.xy) + halfvec);
    float right = depthAtPosition(floor(gl_Vertex.xy + vec2(simplify.x,0.0))  + halfvec );
    float down  = depthAtPosition(floor(gl_Vertex.xy + vec2(0.0,simplify.y))  + halfvec );
    float left  = depthAtPosition(floor(gl_Vertex.xy + vec2(-simplify.x,0.0)) + halfvec );
    float up    = depthAtPosition(floor(gl_Vertex.xy + vec2(0.0,-simplify.y)) + halfvec );
    float bl    = depthAtPosition(vec2(floor(gl_Vertex.x - simplify.x),floor( gl_Vertex.y + simplify.y)) + halfvec );
    float ur    = depthAtPosition(vec2(floor(gl_Vertex.x  + simplify.x),floor(gl_Vertex.y - simplify.y)) + halfvec );

	//cull invalid verts
    positionValid = (depth < farClip &&
					 right < farClip &&
					 down < farClip &&
					 left < farClip &&
					 up < farClip &&
					 bl < farClip &&
					 ur < farClip &&
					 
					 depth > nearClip &&
					 right > nearClip &&
					 down > nearClip &&
					 left > nearClip &&
					 up > nearClip &&
					 bl > nearClip &&
					 ur > nearClip &&
					 
					 abs(down - depth) < edgeClip &&
					 abs(right - depth) < edgeClip &&
					 abs(up - depth) < edgeClip &&
					 abs(left - depth) < edgeClip &&
					 abs(ur - depth) < edgeClip &&
					 abs(bl - depth) < edgeClip
					 ) ? 1.0 : 0.0;
	

	vec4 pos = vec4((gl_Vertex.x - principalPoint.x) * depth / fov.x,
                    (gl_Vertex.y - principalPoint.y) * depth / fov.y, depth, 1.0);
	
	
    //projective texture on the geometry
	//http://opencv.willowgarage.com/documentation/camera_calibration_and_3d_reconstruction.html
	vec4 texCd = vec4(0.);
	vec4 projection = extrinsics * pos;// + vec4(shift*dim / textureScale,0,0);
	
	if(projection.z != 0.0) {
		
		vec2 xyp = projection.xy / projection.z;
		float r2 = pow(xyp.x, 2.0) + pow(xyp.y, 2.0);
		float r4 = r2*r2;
		float r6 = r4*r2;
		vec2 xypp = xyp;
		xypp.x = xyp.x * (1.0 + dK.x*r2 + dK.y*r4 + dK.z*r6) + 2.0*dP.x * xyp.x * xyp.y + dP.y*(r2 + 2.0 * pow(xyp.x,2.0) );
		xypp.y = xyp.y * (1.0 + dK.x*r2 + dK.y*r4 + dK.z*r6) + dP.x * (r2 + 2.0*pow(xyp.y, 2.0) ) + 2.0*dP.y*xyp.x*xyp.y;
		vec2 uv = (colorFOV * xypp + colorPP) * textureScale;
		texCd.xy = ((uv-dim/2.0) * scale) + dim/2.0;
	}
	
	gl_TexCoord[0] = texCd;
    gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * pos;
    gl_FrontColor = gl_Color;
	
}
						  );

const GLchar* frag = GLSL(110, GL_ARB_texture_rectangle : enable,

uniform sampler2DRect colorTex;
varying float positionValid;
const float epsilon = 1e-6;

void main()
{

	if(positionValid < epsilon){
    	discard;
        return;
    }

	gl_FragColor = texture2DRect(colorTex, gl_TexCoord[0].st) * gl_Color;
}
);

using namespace ofxCv;
using namespace cv;

ofxRGBDGPURenderer::ofxRGBDGPURenderer()
	: ofxRGBDRenderer()
{
	rendererBound = false;
	depthOnly = false;
	bShaderLoaded = false;
}

ofxRGBDGPURenderer::~ofxRGBDGPURenderer(){

}


void ofxRGBDGPURenderer::setSimplification(ofVec2f simplification){
	
	if(!calibrationSetup){
		return;
	}

	if(simplify == simplification){
		return;
	}
	
	if(!bShaderLoaded){
		setupDefaultShader();
	}

	if(simplification.x <= 0  || simplification.y <= 0){
		return;
	}
	
	simplify = simplification;
	
	mesh.clearIndices();
	int x = 0;
	int y = 0;
	
	int gw = ceil(depthImageSize.width / simplify.x);
	int w = gw*simplify.x;
	int h = depthImageSize.height;
	
	for (float ystep = 0; ystep < h-simplify.y; ystep += simplify.y){
		for (float xstep = 0; xstep < w-simplify.x; xstep += simplify.x){
			ofIndexType a,b,c;
			
			a = x+y*gw;
			b = (x+1)+y*gw;
			c = x+(y+1)*gw;
			mesh.addIndex(a);
			mesh.addIndex(b);
			mesh.addIndex(c);

			a = (x+1)+(y+1)*gw;
			b = x+(y+1)*gw;
			c = (x+1)+(y)*gw;
			mesh.addIndex(a);
			mesh.addIndex(b);
			mesh.addIndex(c);
			
			x++;
		}
		
		y++;
		x = 0;
	}
	
	mesh.clearVertices();
	for (float y = 0; y < depthImageSize.height; y += simplify.y){
		for (float x = 0; x < depthImageSize.width; x += simplify.x){
			mesh.addVertex(ofVec3f(x,y,0));
		}
	}

	if(addColors){
		mesh.clearColors();
		for (float y = 0; y < depthImageSize.height; y += simplify.y){
			for (float x = 0; x < depthImageSize.width; x += simplify.x){
				mesh.addColor(ofFloatColor(1.0,1.0,1.0,1.0));
			}
		}		
	}
	
	meshGenerated = true;
	
}


void ofxRGBDGPURenderer::setDepthImage(ofShortPixels& pix){
	ofxRGBDRenderer::setDepthImage(pix);
	
	if(!depthTexture.isAllocated() ||
	   depthTexture.getWidth() != pix.getWidth() ||
	   depthTexture.getHeight() != pix.getHeight())
	{
		ofTextureData texData;
		texData.width = pix.getWidth();
		texData.height = pix.getHeight();
		texData.glTypeInternal = GL_LUMINANCE16;
#if OF_VERSION_MINOR < 8
		texData.glType = GL_LUMINANCE;
		texData.pixelType = GL_UNSIGNED_SHORT;
#endif
		depthTexture.allocate(texData);
		depthTexture.bind();
		GLint internalFormat;
		glGetTexLevelParameteriv(texData.textureTarget, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
		depthTexture.unbind();
		
		cout << " is depth texture allocated? " << (depthTexture.bAllocated() ? "yes" : "no") << " internal format? " << internalFormat << " vs " << GL_LUMINANCE16 << endl;
	}
	
}


ofTexture& ofxRGBDGPURenderer::getDepthTexture(){
	return depthTexture;
}

void ofxRGBDGPURenderer::update(){
	
	if(!hasDepthImage){
	 	ofLogError("ofxRGBDGPURenderer::update() -- no depth image");
		return;
	}

	if(!calibrationSetup && hasRGBImage && !depthOnly){
	 	ofLogError("ofxRGBDGPURenderer::update() -- no calibration for RGB Image");
		return;
	}
	
	if(simplify == ofVec2f(0,0)){
		setSimplification(ofVec2f(1.0, 1.0));
	}
	
	depthTexture.loadData(*currentDepthImage);
}

void ofxRGBDGPURenderer::setShaderPath(string path){
	shaderPath = path;
	reloadShader();
}

void ofxRGBDGPURenderer::reloadShader(){
	bShaderLoaded = false;
	if(shaderPath != "" && ofFile(shaderPath + ".vert").exists() && ofFile(shaderPath + ".frag").exists()){
		bShaderLoaded = shader.load(shaderPath);
		if(!bShaderLoaded){
			ofLogError("ofxRGBDGPURenderer::reloadShader") << "Failed to load vert & frag shader at path " << shaderPath;
			setupDefaultShader();
		}
	}
	else{
		setupDefaultShader();
	}
}

void ofxRGBDGPURenderer::setupDefaultShader(){	
	shader.setupShaderFromSource(GL_VERTEX_SHADER, vert);
	shader.setupShaderFromSource(GL_FRAGMENT_SHADER, frag);
	bShaderLoaded = shader.linkProgram();
}

bool ofxRGBDGPURenderer::bindRenderer(){

	if(!hasDepthImage){
	 	ofLogError("ofxRGBDGPURenderer::update() -- no depth image");
		return false;
	}
	
	if(!calibrationSetup && !depthOnly){
	 	ofLogError("ofxRGBDGPURenderer::update() -- no calibration");
		return false;
	}
	
	ofPushMatrix();
	
	ofScale(1, -1, 1);
	if(!mirror){
		ofScale(-1, 1, 1);	
	}
	
	ofRotate(worldRotation.x,1,0,0);
	ofRotate(worldRotation.y,0,1,0);
	ofRotate(worldRotation.z,0,0,1);


	shader.begin();
	glActiveTexture(GL_TEXTURE1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glActiveTexture(GL_TEXTURE0);

	setupProjectionUniforms();
	
	rendererBound = true;
	return true;
}

void ofxRGBDGPURenderer::unbindRenderer(){
	
	if(!rendererBound){
		ofLogError("ofxRGBDGPURenderer::unbindRenderer -- called without renderer bound");
	 	return;   
	}
	
	shader.end();
	rendererBound = false;

	ofPopMatrix();
}

void ofxRGBDGPURenderer::setupProjectionUniforms(){

	if(!depthOnly && useTexture){
		
		//cout << " adjust mat" << endl << getAdjustedMatrix() << endl;
		
		ofVec2f dims = ofVec2f(currentRGBImage->getTextureReference().getWidth(),
							   currentRGBImage->getTextureReference().getHeight());
		shader.setUniformTexture("colorTex", currentRGBImage->getTextureReference(), 0);
		shader.setUniform1i("useTexture", 1);
		shader.setUniform2f("dim", dims.x, dims.y);
		shader.setUniform2f("textureScale", textureScale.x, textureScale.y);
		shader.setUniform2f("shift", shift.x, shift.y);
		shader.setUniform2f("scale", scale.x, scale.y);
		shader.setUniform3f("dK", distortionK.x, distortionK.y, distortionK.z);
		shader.setUniform2f("dP", distortionP.x, distortionP.y);
	
		shader.setUniformMatrix4f( "extrinsics", getAdjustedMatrix() );

		shader.setUniform2f("colorFOV", colorFOV.x, colorFOV.y );
		shader.setUniform2f("colorPP", colorPrincipalPoint.x, colorPrincipalPoint.y);
	}
	else{
		shader.setUniform1i("useTexture", 0);
	}
	
	shader.setUniformTexture("depthTex", depthTexture, 1);
	shader.setUniform2f("principalPoint", depthPrincipalPoint.x, depthPrincipalPoint.y);
	
	shader.setUniform2f("fov", depthFOV.x, depthFOV.y);
	shader.setUniform1f("nearClip", nearClip);
	shader.setUniform1f("farClip", farClip);
	shader.setUniform1f("edgeClip", edgeClip);
	
	shader.setUniform1f("leftClip",leftClip);
	shader.setUniform1f("rightClip",rightClip);
	shader.setUniform1f("topClip",topClip);
	shader.setUniform1f("bottomClip",bottomClip);
	
	shader.setUniform2f("simplify", simplify.x, simplify.y);
	
//	printUniforms();
}

void ofxRGBDGPURenderer::printUniforms(){
	bool color = !depthOnly && useTexture;
	ofVec2f dims;
	if(color){
		dims = ofVec2f(currentRGBImage->getTextureReference().getWidth(),
					   currentRGBImage->getTextureReference().getHeight());
	}

	cout << "UNIFORMS " << endl;
	cout << "	 useTexture " << (color ? "YES" : "NO") << endl;
	cout << "	 dim " << dims << endl;
	cout << "	 texture scale " << textureScale << endl;
	cout << "	 shift " << shift << endl;
	cout << "	 scale " << scale << endl;
	cout << "	 distortion K " << distortionK << endl;
	cout << "	 distortion P " << distortionP << endl;
	cout << "	 color FOV " << colorFOV << endl;
	cout << "	 color PP " << colorPrincipalPoint << endl;

	cout << "	 depthPP " << depthPrincipalPoint << endl;
	cout << "	 depth FOV " << depthFOV << endl;
	
	cout << "	 near clip " << nearClip << endl;
	cout << "	 far clip " << farClip << endl;
	cout << "	 edge clip " << edgeClip << endl;
	
	cout << "	 left clip " << leftClip << endl;
	cout << "	 right clip " << rightClip << endl;
	cout << "	 top clip " << topClip << endl;
	cout << "	 bottom clip " << bottomClip << endl;
	cout << "	 simplify " << simplify << endl;
	
	cout << "	 extrinsics " << endl << getAdjustedMatrix() << endl;

}

ofShader& ofxRGBDGPURenderer::getShader(){
	return shader;
}

void ofxRGBDGPURenderer::draw(ofPolyRenderMode drawMode){
	if(bindRenderer()){
		
		switch(drawMode){
			case OF_MESH_POINTS:
				mesh.drawVertices(); break;
			case OF_MESH_WIREFRAME:
				mesh.drawWireframe(); break;
			case OF_MESH_FILL:
				mesh.drawFaces(); break;
		}
		
		unbindRenderer();
	}
	else{
		ofLogError("ofxRGBDGPURenderer::draw") << "Binde renderer failed";
	}
}

