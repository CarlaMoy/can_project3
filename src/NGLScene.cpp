#include <QMouseEvent>
#include <QGuiApplication>

#include "NGLScene.h"
#include <ngl/Camera.h>
#include <ngl/Light.h>
#include <ngl/Material.h>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>
#include <ngl/VAOFactory.h>
#include <ngl/MultiBufferVAO.h>
#include <array>
#include <glm/gtc/type_ptr.hpp>
#include <noise/noise.h>



constexpr int TEXTURE_WIDTH=1024;
constexpr int TEXTURE_HEIGHT=1024;

constexpr auto CanProgram="CanProgram";
constexpr auto PlaneProgram="PlaneProgram";


NGLScene::NGLScene()
{
  m_animate=true;
  m_lightPosition.set(8,4,8);
  m_lightYPos=4.0;
  m_lightXoffset=8.0;
  m_lightZoffset=8.0;
  setTitle("Can Project");
}

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//

NGLScene::~NGLScene()
{
  std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";
}

void NGLScene::resizeGL( int _w, int _h )
{
  m_cam.setShape( 45.0f, static_cast<float>( _w ) / _h, 0.05f, 350.0f );
  m_win.width  = static_cast<int>( _w * devicePixelRatio() );
  m_win.height = static_cast<int>( _h * devicePixelRatio() );
}

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//

void NGLScene::initializeGL()
{
  constexpr float znear=0.1f;
  constexpr float zfar=100.0f;
  // we must call this first before any other GL commands to load and link the
  // gl commands from the lib, if this is not done program will crash
  ngl::NGLInit::instance();

  glClearColor(0.4f, 0.4f, 0.4f, 1.0f);			   // Grey Background
  // enable depth testing for drawing
  glEnable(GL_DEPTH_TEST);
  // enable multisampling for smoother drawing
  glEnable(GL_MULTISAMPLE);


  m_quadVAO = 0;


  // now to load the shader and set the values
  // grab an instance of shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();

  // Now we will create a basic Camera from the graphics library
  // This is a static camera so it only needs to be set once
  // First create Values for the camera position
  ngl::Vec3 from(0,1,4);
  ngl::Vec3 to(0,1,0);
  ngl::Vec3 up(0,1,0);
  // now load to our new camera
  m_cam.set(from,to,up);
  // set the shape using FOV 45 Aspect Ratio based on Width and Height
  // The final two are near and far clipping planes of 0.5 and 10
  m_cam.setShape(45,720.0f/576.0f,znear,zfar);

  // now load to our light POV camera
  m_lightCamera.set(m_lightPosition,to,up);
  // here we set the light POV camera shape, the aspect is 1 as our
  // texture is square.
  // use the same clip plane as before but set the FOV a bit larger
  // to get slightly better shadows and the clip planes will help
  // to get rid of some of the artefacts
  m_lightCamera.setShape(45,float(width()/height()),znear,zfar);


  // in this case I'm only using the light to hold the position
  // it is not passed to the shader directly
  m_lightAngle=0.0f;

  //________________________________________________________________________________________________________________________________________//

  // Initialise our environment map here
  initEnvironment();

  // Initialise texture maps here
  initTexture(1, m_glossMapTex, "images/gloss.png");
  initTexture(2, m_labelTex, "images/colourMapCan.tif");
  initTexture(3, m_bumpTex, "images/NormalMap.jpg");

  initTexture(4, m_woodTex, "images/woodDif.jpg");
  initTexture(5, m_woodSpec, "images/woodSpec.jpg");
  initTexture(6, m_woodNorm, "images/woodNorm.jpg");


  //________________________________________________________________________________________________________________________________________//


  //________________________________________________________________________________________________________________________________________//

  // we are creating a shader called DOF
  shader->createShaderProgram("DOF");
  // now we are going to create empty shaders for Frag and Vert
  shader->attachShader("DOFVertex",ngl::ShaderType::VERTEX);
  shader->attachShader("DOFFragment",ngl::ShaderType::FRAGMENT);
  // attach the source
  shader->loadShaderSource("DOFVertex","shaders/depthOfFieldVert.glsl");
  shader->loadShaderSource("DOFFragment","shaders/depthOfFieldFrag.glsl");
  // compile the shaders
  shader->compileShader("DOFVertex");
  shader->compileShader("DOFFragment");
  // add them to the program
  shader->attachShaderToProgram("DOF","DOFVertex");
  shader->attachShaderToProgram("DOF","DOFFragment");

  // now we have associated this data we can link the shader
  shader->linkProgramObject("DOF");

  //________________________________________________________________________________________________________________________________________//


  // we are creating a shader called DOFFinal
  shader->createShaderProgram("DOFFinal");
  // now we are going to create empty shaders for Frag and Vert
  shader->attachShader("DOFFinalVertex",ngl::ShaderType::VERTEX);
  shader->attachShader("DOFFinalFragment",ngl::ShaderType::FRAGMENT);
  // attach the source
  shader->loadShaderSource("DOFFinalVertex","shaders/DOFFinalVert.glsl");
  shader->loadShaderSource("DOFFinalFragment","shaders/DOFFinalFrag.glsl");
  // compile the shaders
  shader->compileShader("DOFFinalVertex");
  shader->compileShader("DOFFinalFragment");
  // add them to the program
  shader->attachShaderToProgram("DOFFinal","DOFFinalVertex");
  shader->attachShaderToProgram("DOFFinal","DOFFinalFragment");

  // now we have associated this data we can link the shader
  shader->linkProgramObject("DOFFinal");

  //________________________________________________________________________________________________________________________________________//

  // we are creating a shader called Colour
  shader->createShaderProgram("Colour");
  // now we are going to create empty shaders for Frag and Vert
  shader->attachShader("ColourVertex",ngl::ShaderType::VERTEX);
  shader->attachShader("ColourFragment",ngl::ShaderType::FRAGMENT);
  // attach the source
  shader->loadShaderSource("ColourVertex","shaders/ColourVert.glsl");
  shader->loadShaderSource("ColourFragment","shaders/ColourFrag.glsl");
  // compile the shaders
  shader->compileShader("ColourVertex");
  shader->compileShader("ColourFragment");
  // add them to the program
  shader->attachShaderToProgram("Colour","ColourVertex");
  shader->attachShaderToProgram("Colour","ColourFragment");

  // now we have associated this data we can link the shader
  shader->linkProgramObject("Colour");
  shader->use("Colour");

  shader->setShaderParam4f("Colour",1,0,0,1);


  //________________________________________________________________________________________________________________________________________//

  // we are creating a shader called Shadow
  shader->createShaderProgram("Shadow");
  // now we are going to create empty shaders for Frag and Vert
  shader->attachShader("ShadowVertex",ngl::ShaderType::VERTEX);
  shader->attachShader("ShadowFragment",ngl::ShaderType::FRAGMENT);
  // attach the source
  shader->loadShaderSource("ShadowVertex","shaders/ShadowVert.glsl");
  shader->loadShaderSource("ShadowFragment","shaders/ShadowFrag.glsl");
  // compile the shaders
  shader->compileShader("ShadowVertex");
  shader->compileShader("ShadowFragment");
  // add them to the program
  shader->attachShaderToProgram("Shadow","ShadowVertex");
  shader->attachShaderToProgram("Shadow","ShadowFragment");

  // now we have associated this data we can link the shader
  shader->linkProgramObject("Shadow");

  shader->use("Shadow");

  shader->setUniform("specMap", 4);
  shader->setUniform("difMap", 5);
  shader->setUniform("normMap", 6);

  // shader->use("Shadow");

  // initTexture(1, m_textureMap, "images/wood.jpg");

  // shader->setUniform("textureMap", 1);

  // create the primitives to draw
  ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();

  prim->createTrianglePlane("plane",80,80,80,80,ngl::Vec3(0,1,0));


  //________________________________________________________________________________________________________________________________________//

  shader->loadShader(CanProgram,
                     "shaders/CanVert.glsl",
                     "shaders/CanFrag.glsl");
  shader->getProgramID(CanProgram);


  shader->use(CanProgram);
  m_CanID = shader->getProgramID(CanProgram);
  glUniform1i(glGetUniformLocation(m_CanID, "gPosition"), 3);
  glUniform1i(glGetUniformLocation(m_CanID, "gNormal"), 4);
  glUniform1i(glGetUniformLocation(m_CanID, "gColour"), 5);
  glUniform1i(glGetUniformLocation(m_CanID, "ssao"), 6);


  shader->setUniform("glossMap", 1);
  shader->setUniform("labelMap", 2);
  shader->setUniform("normalMap", 3);

  shader->setShaderParam3f("Light[0].La", 0.5, 0.5, 0.5);
  shader->setShaderParam3f("Light[0].Ld", 1.0, 1.0, 1.0);
  shader->setShaderParam3f("Light[0].Ls", 1.0, 1.0, 1.0);
  shader->setShaderParam3f("Light[0].Intensity", 1.0, 1.0, 1.0);
  shader->setShaderParam1f("Light[0].Linear", 0.0014);
  shader->setShaderParam1f("Light[0].Quadratic", 0.000007);
  shader->setShaderParam4f("Light[1].Position",-2.0, 2.0, 4.0, 1.0);
  shader->setShaderParam3f("Light[1].La", 0.5, 0.5, 0.5);
  shader->setShaderParam3f("Light[1].Ld", 0.1, 1.0, 1.0);
  shader->setShaderParam3f("Light[1].Ls", 1.0, 1.0, 1.0);
  shader->setShaderParam3f("Light[1].Intensity", 1.0, 1.0, 4.0);
  shader->setShaderParam1f("Light[1].Linear", 0.35);
  shader->setShaderParam1f("Light[1].Quadratic", 0.44);
  shader->setShaderParam4f("Light[2].Position",4.0, 3.0, -1.0, 0.0);
  shader->setShaderParam3f("Light[2].La", 0.5, 0.5, 0.5);
  shader->setShaderParam3f("Light[2].Ld", 1.0, 0.1, 1.0);
  shader->setShaderParam3f("Light[2].Ls", 1.0, 1.0, 1.0);
  shader->setShaderParam3f("Light[2].Intensity", 5.0, 0.6, 0.6);
  shader->setShaderParam1f("Light[2].Linear", 0.7);
  shader->setShaderParam1f("Light[2].Quadratic", 1.8); //linear and quadratic values for attenuation from
  //http://www.ogre3d.org/tikiwiki/tiki-index.php?page=-Point+Light+Attenuation

  shader->setShaderParam2f("iResolution", width(), height());

  // Load the Obj file and create a Vertex Array Object
  m_mesh.reset(new ngl::Obj("data/can05.obj"));
  m_mesh->createVAO();


  //________________________________________________________________________________________________________________________________________//


  // create shadow FBO
  createShadowFBO();

  //create SSAO FBOs and Kernel noise for SSAO
 // CreateGBuffer();

  //createSSAOBuffers();

 // createSSAOKernelNoise();

  createBlurFBO();



  //________________________________________________________________________________________________________________________________________//



  // we need to enable depth testing
  glEnable(GL_DEPTH_TEST);
  // set the depth comparison mode
  glDepthFunc(GL_LEQUAL);
  // set the bg to black
  glClearColor(0,0,0,1.0f);
  // enable face culling this will be switch to front and back when
  // rendering shadow or scene
  glEnable(GL_CULL_FACE);
  glPolygonOffset(1.1f,4);
  m_text.reset(  new ngl::Text(QFont("Ariel",14)));
  m_text->setColour(1,1,1);
  // as re-size is not explicitly called we need to do this.
  // also need to take into account the retina display
  glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());
  m_lightTimer =startTimer(40);

  //glEnable(GL_FRAMEBUFFER_SRGB);

}

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//

void NGLScene::loadMatricesToShadowShader()
{
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  shader->use("Shadow");
  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat3 normalMatrix;
  ngl::Mat4 M;
  M=m_transform.getMatrix()*m_mouseGlobalTX;
  MV=  M*m_cam.getViewMatrix();
  MVP= M*m_cam.getVPMatrix();
  normalMatrix=MV;
  normalMatrix = normalMatrix.inverse();
  // shader->setShaderParamFromMat4("M",M);
  // shader->setShaderParamFromMat4("MV",MV);
  shader->setShaderParamFromMat4("MVP",MVP);
  shader->setShaderParamFromMat3("normalMatrix",normalMatrix);
  shader->setShaderParam4f("Light[0].Position",m_lightPosition.m_x,m_lightPosition.m_y,m_lightPosition.m_z, 1.0);
  shader->setShaderParam3f("Light[0].La", 0.5, 0.5, 0.5);
  shader->setShaderParam3f("Light[0].Ld", 1.0, 1.0, 1.0);
  shader->setShaderParam3f("Light[0].Ls", 1.0, 1.0, 1.0);
  shader->setShaderParam3f("Light[0].Intensity", 1.0, 1.0, 1.0);
  shader->setShaderParam1f("Light[0].Linear", 0.0014);
  shader->setShaderParam1f("Light[0].Quadratic", 0.000007);
  shader->setShaderParam4f("Light[1].Position",0.0, 2.0, 4.0, 1.0);
  shader->setShaderParam3f("Light[1].La", 0.5, 0.5, 0.5);
  shader->setShaderParam3f("Light[1].Ld", 0.1, 1.0, 1.0);
  shader->setShaderParam3f("Light[1].Ls", 1.0, 1.0, 1.0);
  shader->setShaderParam3f("Light[1].Intensity", 1.0, 1.0, 4.0);
  shader->setShaderParam1f("Light[1].Linear", 0.35);
  shader->setShaderParam1f("Light[1].Quadratic", 1.44);
  shader->setShaderParam4f("Light[2].Position",4.0, 3.0, -1.0, 0.0);
  shader->setShaderParam3f("Light[2].La", 0.5, 0.5, 0.5);
  shader->setShaderParam3f("Light[2].Ld", 1.0, 0.1, 1.0);
  shader->setShaderParam3f("Light[2].Ls", 1.0, 1.0, 1.0);
  shader->setShaderParam3f("Light[2].Intensity", 5.0, 0.6, 0.6);
  shader->setShaderParam1f("Light[2].Linear", 0.7);
  shader->setShaderParam1f("Light[2].Quadratic", 1.8);


  // shader->setShaderParam4f("inColour",1,1,1,1);

  // x = x* 0.5 + 0.5
  // y = y* 0.5 + 0.5
  // z = z* 0.5 + 0.5
  // Moving from unit cube [-1,1] to [0,1]
  ngl::Mat4 bias;
  bias.scale(0.5,0.5,0.5);
  bias.translate(0.5,0.5,0.5);

  ngl::Mat4 view=m_lightCamera.getViewMatrix();
  ngl::Mat4 proj=m_lightCamera.getProjectionMatrix();
  ngl::Mat4 model=m_transform.getMatrix();

  //  ngl::Mat4 lightSpaceMatrix = model * view * proj * bias;

  ngl::Mat4 textureMatrix= model * view*proj * bias;
  shader->setShaderParamFromMat4("textureMatrix",textureMatrix);
  // shader->setShaderParamFromMat4("lightSpaceMatrix",lightSpaceMatrix);
}

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//

void NGLScene::loadToLightPOVShader()
{
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  shader->use("Colour");
  ngl::Mat4 MVP=m_transform.getMatrix()* m_lightCamera.getVPMatrix();
  shader->setShaderParamFromMat4("MVP",MVP);
}

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//

void NGLScene::drawScene(std::function<void()> _shaderFunc )
{
  // Rotation based on the mouse position for our global transform
  ngl::Mat4 rotX;
  ngl::Mat4 rotY;
  // create the rotation matrices
  rotX.rotateX(m_win.spinXFace);
  rotY.rotateY(m_win.spinYFace);
  // multiply the rotations
  m_mouseGlobalTX=rotY*rotX;
  // add the translations
  m_mouseGlobalTX.m_m[3][0] = m_modelPos.m_x;
  m_mouseGlobalTX.m_m[3][1] = m_modelPos.m_y;
  m_mouseGlobalTX.m_m[3][2] = m_modelPos.m_z;

  //________________________________________________________________________________________________________________________________________//

  // get the VBO instance
  ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();


  m_transform.reset();
  m_transform.setPosition(0.0f,0.0f,0.0f);
  _shaderFunc();
  prim->draw("plane");
  //________________________________________________________________________________________________________________________________________//

  m_transform.reset();
  m_transform.setPosition(0.0f,0.0f,0.0f);
  m_transform.setScale(0.4,0.4,0.4);
  _shaderFunc();
  m_mesh->draw();

}

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//

void NGLScene::paintGL()
{
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();


  //________________________________________________________________________________________________________________________________________//

  //----------------------------------------------------------------------------------------------------------------------
  // Pass 1 render the Depth texture to the FBO
  //----------------------------------------------------------------------------------------------------------------------
  // enable culling
  glEnable(GL_CULL_FACE);

  // bind the FBO and render offscreen to the texture
  glBindFramebuffer(GL_FRAMEBUFFER,m_ShadowfboID);
  // bind the texture object to 0 (off )
  glBindTexture(GL_TEXTURE_2D,0);
  // render to the same size as the texture to avoid
  // distortions
  glViewport(0,0,TEXTURE_WIDTH,TEXTURE_HEIGHT);

  // Clear previous frame values
  glClear( GL_DEPTH_BUFFER_BIT);
  // only rendering depth, turn off the colour / alpha
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

  // render only the back faces so less self shadowing
  glCullFace(GL_FRONT);
  // draw the scene from the POV of the light
  drawScene(std::bind(&NGLScene::loadToLightPOVShader,this));

  //________________________________________________________________________________________________________________________________________//

  //----------------------------------------------------------------------------------------------------------------------
  // Pass two : use the shadow map texture
  // Render the scene with the shadow map texture on the ground plane
  //----------------------------------------------------------------------------------------------------------------------
  // store framebuffer for main scene to a texture
  glBindFramebuffer(GL_FRAMEBUFFER, m_blurFBO);

  // set the viewport to the screen dimensions
  glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());
  // enable colour rendering again
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  // clear the screen
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(0.5f, 0.5f, 0.6f, 1.0f);

  // bind the shadow texture
  glBindTexture(GL_TEXTURE_2D,m_ShadowtextureID);


  // only cull back faces
  glDisable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  // render scene with the shadow shader
  drawScene(std::bind(&NGLScene::loadMatricesToShadowShader,this));

  //Draw the can

  m_transform.reset();
  m_transform.setPosition(0.0f,0.0f,0.0f);
  m_transform.setScale(0.4,0.4,0.4);
  loadMatrices(CanProgram);
  shader->use(CanProgram);
  shader->setShaderParam4f("Light[0].Position",m_lightPosition.m_x,m_lightPosition.m_y,m_lightPosition.m_z, 1.0);
  m_mesh->draw();


  //________________________________________________________________________________________________________________________________________//

  //----------------------------------------------------------------------------------------------------------------------
  // Pass three : Ping Pong Framebuffers for blur effect
  // Render the previous scene's stored texture to a screen aligned quad and blur
  //----------------------------------------------------------------------------------------------------------------------

 //Code taken from https://learnopengl.com/#!Advanced-Lighting/Bloom
  bool horizontal = true, first_iteration = true;
  unsigned int amount = 30;
  shader->use("DOF");
  for (unsigned int i = 0; i < amount; i++)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, m_pingpongFBO[horizontal]);
    shader->setRegisteredUniform1i("horizontal", horizontal);
    glBindTexture(GL_TEXTURE_2D, first_iteration ? m_blurTexFBO : m_pingpongColourBuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_blurDepthFBO);
    RenderQuad();
    horizontal = !horizontal;
    if (first_iteration)
      first_iteration = false;
  }
  //End of code taken from https://learnopengl.com/#!Advanced-Lighting/Bloom

  //----------------------------------------------------------------------------------------------------------------------
  // Pass four : Render to default Framebuffer
  //----------------------------------------------------------------------------------------------------------------------

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  shader->use("DOFFinal");

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_pingpongColourBuffers[0]);

  RenderQuad();
}

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//


void NGLScene::createShadowFBO()
{


  glGenTextures(1, &m_ShadowtextureID);
  glBindTexture(GL_TEXTURE_2D, m_ShadowtextureID);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

  glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

  glBindTexture(GL_TEXTURE_2D, 0);

  // create FBO
  glGenFramebuffers(1, &m_ShadowfboID);
  glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowfboID);
  // disable the colour and read buffers as only depth needed
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);

  // attach texture to the FBO

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D, m_ShadowtextureID, 0);

  // switch back to default framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//

void NGLScene::createBlurFBO()
{
    //Code taken from myBU: Principles of Rendering, Unit Materials, Real Time Rendering, W6_DepthOfField
    // First delete the FBO if it has been created previously
    glBindFramebuffer(GL_FRAMEBUFFER, m_blurFBO);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE) {
        glDeleteTextures(1, &m_blurTexFBO);
        glDeleteTextures(1, &m_blurDepthFBO);
        glDeleteFramebuffers(1, &m_blurFBO);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Generate a texture to write the FBO result to
    glGenTextures(1, &m_blurTexFBO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_blurTexFBO);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB,
                 width(),
                 height(),
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // The depth buffer is rendered to a texture buffer
    glGenTextures(1, &m_blurDepthFBO);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_blurDepthFBO);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_DEPTH_COMPONENT,
                 width(),
                 height(),
                 0,
                 GL_DEPTH_COMPONENT,
                 GL_UNSIGNED_BYTE,
                 NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create the frame buffer
    glGenFramebuffers(1, &m_blurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_blurFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_blurTexFBO, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_blurDepthFBO, 0);

    // Set the fragment shader output targets (DEPTH_ATTACHMENT is done automatically)
    GLenum drawBufs[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, drawBufs);

  //Code taken from https://learnopengl.com/#!Advanced-Lighting/Bloom
  // ping-pong-framebuffer for blurring
  glGenFramebuffers(2, m_pingpongFBO);
  glGenTextures(2, m_pingpongColourBuffers);
  for (unsigned int i = 0; i < 2; i++)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, m_pingpongFBO[i]);
    glBindTexture(GL_TEXTURE_2D, m_pingpongColourBuffers[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width(), height(), 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // clamp to the edge as the blur filter would otherwise sample repeated texture values
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pingpongColourBuffers[i], 0);
    // also check if framebuffers are complete (no need for depth buffer)
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      std::cout << "Framebuffer not complete!" << std::endl;
  }
  //End of code taken from https://learnopengl.com/#!Advanced-Lighting/Bloom
}

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//


void NGLScene::keyPressEvent(QKeyEvent *_event)
{
  // this method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  switch (_event->key())
  {
  // escape key to quite
  case Qt::Key_Escape : QGuiApplication::exit(EXIT_SUCCESS); break;
    // turn on wirframe rendering
  case Qt::Key_W : glPolygonMode(GL_FRONT_AND_BACK,GL_LINE); break;
    // turn off wire frame
  case Qt::Key_S : glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); break;
    // show full screen
  case Qt::Key_F : showFullScreen(); break;
    // show windowed
  case Qt::Key_N : showNormal(); break;
  case Qt::Key_Space : toggleAnimation(); break;
  case Qt::Key_Left : changeLightXOffset(-0.1f); break;
  case Qt::Key_Right : changeLightXOffset(0.1f); break;
  case Qt::Key_Up : changeLightYPos(0.1f); break;
  case Qt::Key_Down : changeLightYPos(-0.1f); break;
  case Qt::Key_I : changeLightZOffset(-0.1f); break;
  case Qt::Key_O : changeLightZOffset(0.1f); break;

  default : break;
  }

  update();
}

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//

void NGLScene::updateLight()
{
  // change the light angle
  m_lightAngle+=0.02;
  m_lightPosition.set(m_lightXoffset*cos(m_lightAngle),m_lightYPos,m_lightXoffset*sin(m_lightAngle));
  // set this value and load to the shader
  m_lightCamera.set(m_lightPosition,ngl::Vec3(0,0,0),ngl::Vec3(0,1,0));
}

//________________________________________________________________________________________________________________________________________//

void NGLScene::timerEvent(QTimerEvent *_event )
{
  // if the timer is the light timer call the update light method
  if(_event->timerId() == m_lightTimer && m_animate==true)
  {
    updateLight();
  }
  // re-draw GL
  update();
}

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//

void NGLScene::debugTexture(float _t, float _b, float _l, float _r)
{
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  shader->use("Shadow");
  ngl::Mat4 MVP=1;
  shader->setShaderParamFromMat4("MVP",MVP);
  glBindTexture(GL_TEXTURE_2D,m_ShadowtextureID);

  std::unique_ptr<ngl::AbstractVAO> quad(ngl::VAOFactory::createVAO("multiBufferVAO",GL_TRIANGLES));
  std::array<float,18> vert ;	// vertex array
  std::array<float,12> uv ;	// uv array
  vert[0] =_t; vert[1] =  _l; vert[2] =0.0;
  vert[3] = _t; vert[4] =  _r; vert[5] =0.0;
  vert[6] = _b; vert[7] = _l; vert[8]= 0.0;

  vert[9] =_b; vert[10]= _l; vert[11]=0.0;
  vert[12] =_t; vert[13]=_r; vert[14]=0.0;
  vert[15] =_b; vert[16]= _r; vert[17]=0.0;

  uv[0] =0.0; uv[1] =  1.0;
  uv[2] = 1.0; uv[3] =  1.0;
  uv[4] = 0.0; uv[5] = 0.0;

  uv[6] =0.0; uv[7]= 0.0;
  uv[8] =1.0; uv[9]= 1.0;
  uv[10] =1.0; uv[11]= 0.0;


  quad->bind();

  quad->setData(ngl::AbstractVAO::VertexData(18*sizeof(GLfloat),vert[0]));
  quad->setVertexAttributePointer(0,3,GL_FLOAT,0,0);
  quad->setData(ngl::AbstractVAO::VertexData(12*sizeof(GLfloat),uv[0]));
  quad->setVertexAttributePointer(1,2,GL_FLOAT,0,0);
  quad->setNumIndices(6);
  quad->draw();
  quad->unbind();
}

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//

void NGLScene::initTexture(const GLuint& texUnit, GLuint &texId, const char *filename) {
  // Set active texture unit
  glActiveTexture(GL_TEXTURE0 + texUnit);

  // Load up the image using NGL routine
  ngl::Image img(filename);

  // Create storage for the new texture
  glGenTextures(1, &texId);

  // Bind the current texture
  glBindTexture(GL_TEXTURE_2D, texId);

  // Transfer image data onto the GPU using the teximage2D call
  glTexImage2D (
        GL_TEXTURE_2D,    // The target (in this case, which side of the cube)
        0,                // Level of mipmap to load
        img.format(),     // Internal format (number of colour components)
        img.width(),      // Width in pixels
        img.height(),     // Height in pixels
        0,                // Border
        GL_RGB,          // Format of the pixel data
        GL_UNSIGNED_BYTE, // Data type of pixel data
        img.getPixels()); // Pointer to image data in memory

  // Set up parameters for our texture
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//

/**
 * @brief Scene::initEnvironment in texture unit 0
 */
void NGLScene::initEnvironment() {
  // Enable seamless cube mapping
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

  // Placing environment map texture in texture unit 0
  glActiveTexture (GL_TEXTURE0);

  // Generate storage and a reference for environment map texture
  glGenTextures (1, &m_envTex);

  // Bind this texture to the active texture unit
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_envTex);

  // Now load up the sides of the cube
  initEnvironmentSide(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, "images/sky_zneg.png");
  initEnvironmentSide(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, "images/sky_zpos.png");
  initEnvironmentSide(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, "images/sky_ypos.png");
  initEnvironmentSide(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, "images/sky_yneg.png");
  initEnvironmentSide(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, "images/sky_xneg.png");
  initEnvironmentSide(GL_TEXTURE_CUBE_MAP_POSITIVE_X, "images/sky_xpos.png");

  // Generate mipmap levels
  glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

  // Set the texture parameters for the cube map
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_GENERATE_MIPMAP, GL_TRUE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  GLfloat anisotropy;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisotropy);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);

  // Set cube map texture to on the shader so we can use it
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  shader->use(CanProgram);
  shader->setUniform("envMap", 0);

}

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//

/**
 * @brief Scene::initEnvironmentSide
 * @param texture
 * @param target
 * @param filename
 * This function should only be called when we have the environment texture bound already
 * copy image data into 'target' side of cube map
 */
void NGLScene::initEnvironmentSide(GLenum target, const char *filename)
{
  // Load up the image using NGL routine
  ngl::Image img(filename);

  // Transfer image data onto the GPU using the teximage2D call
  glTexImage2D (
        target,           // The target (in this case, which side of the cube)
        0,                // Level of mipmap to load
        img.format(),     // Internal format (number of colour components)
        img.width(),      // Width in pixels
        img.height(),     // Height in pixels
        0,                // Border
        GL_RGBA,          // Format of the pixel data
        GL_UNSIGNED_BYTE, // Data type of pixel data
        img.getPixels()   // Pointer to image data in memory
        );
}

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//

void NGLScene::loadMatrices(const std::string _program)
{
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();

  shader->use(_program);
  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat3 normalMatrix;
  ngl::Mat4 M;
  ngl::Mat4 P;
  ngl::Mat4 V;
  M=m_transform.getMatrix()*m_mouseGlobalTX;
  MV=  M*m_cam.getViewMatrix();
  MVP= M*m_cam.getVPMatrix();
  V = m_cam.getViewMatrix();
  P = m_cam.getProjectionMatrix();
  normalMatrix=MV;
  normalMatrix.inverse();
  shader->setShaderParamFromMat4("MV",MV);
  shader->setShaderParamFromMat4("MVP",MVP);
  shader->setShaderParamFromMat3("N",normalMatrix);
  shader->setUniform("viewPos", m_cam.getEye().toVec3());
 }

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//

// RenderQuad() Renders a 1x1 quad in NDC, best used for framebuffer colour targets
// and post-processing effects.

void NGLScene::RenderQuad()
{
  if (m_quadVAO == 0)
  {
    GLfloat quadVertices[] = {
      // Positions        // Texture Coords
      -1,  1, 0.0f, 0.0f, 1.0f,
      -1, -1, 0.0f, 0.0f, 0.0f,
      1,  1, 0.0f, 1.0f, 1.0f,
      1, -1, 0.0f, 1.0f, 0.0f,
    };
    // Setup plane VAO
    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
  }
  glBindVertexArray(m_quadVAO);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindVertexArray(0);
}


//________________________________________________________________________________________________________________________________________//
//========================================================================================================================================//
//unused functions
//========================================================================================================================================//

/*void NGLScene::CreateGBuffer()
{
  glGenFramebuffers(1, &m_gBuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, m_gBuffer);


  // - Position color buffer
  glGenTextures(1, &m_gPosition);
  glBindTexture(GL_TEXTURE_2D, m_gPosition);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width(), height(), 0, GL_RGB, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_gPosition, 0);

  // - Normal color buffer
  glGenTextures(1, &m_gNormal);
  glBindTexture(GL_TEXTURE_2D, m_gNormal);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width(), height(), 0, GL_RGB, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_gNormal, 0);

  // - Color + Specular color buffer
  glGenTextures(1, &m_gColour);
  glBindTexture(GL_TEXTURE_2D, m_gColour);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width(), height(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_gColour, 0);

  // - Tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
  GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
  glDrawBuffers(3, attachments);

  // - Create and attach depth buffer (renderbuffer)
  GLuint rboDepth;
  glGenRenderbuffers(1, &rboDepth);
  glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width(), height());
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
  // - Finally check if framebuffer is complete
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "Framebuffer not complete!" << std::endl;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//

void NGLScene::createSSAOBuffers()
{
  // Also create framebuffer to hold SSAO processing stage

  glGenFramebuffers(1, &m_ssaoFBO);  glGenFramebuffers(1, &m_ssaoBlurFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoFBO);

  // - SSAO color buffer
  glGenTextures(1, &m_ssaoColorBuffer);
  glBindTexture(GL_TEXTURE_2D, m_ssaoColorBuffer);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width(), height(), 0, GL_RGB, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ssaoColorBuffer, 0);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "SSAO Framebuffer not complete!" << std::endl;
  // - and blur stage
  glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoBlurFBO);
  glGenTextures(1, &m_ssaoColorBufferBlur);
  glBindTexture(GL_TEXTURE_2D, m_ssaoColorBufferBlur);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width(), height(), 0, GL_RGB, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ssaoColorBufferBlur, 0);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "SSAO Blur Framebuffer not complete!" << std::endl;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//________________________________________________________________________________________________________________________________________//

GLfloat NGLScene::lerp(GLfloat a, GLfloat b, GLfloat f)
{
  return a + f * (b - a);
}

//________________________________________________________________________________________________________________________________________//

void NGLScene::createSSAOKernelNoise()
{
  // Sample kernel
  std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
  std::default_random_engine generator;

  for (GLuint i = 0; i < 64; ++i)
  {
    glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
    sample = glm::normalize(sample);
    sample *= randomFloats(generator);
    GLfloat scale = GLfloat(i) / 64.0;

    // Scale samples s.t. they're more aligned to center of kernel
    scale = lerp(0.1f, 1.0f, scale * scale);
    sample *= scale;
    m_ssaoKernel.push_back(sample);

    // Noise texture
    std::vector<glm::vec3> ssaoNoise;
    for (GLuint i = 0; i < 16; i++)
    {
      glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
      ssaoNoise.push_back(noise);
    }
    glGenTextures(1, &m_noiseTexture);
    glBindTexture(GL_TEXTURE_2D, m_noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  }
}*/
//========================================================================================================================================//
//========================================================================================================================================//

