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
  ngl::Vec3 from(0,3,6);
  ngl::Vec3 to(0,0,0);
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
  initTexture(3, m_bumpTex, "images/bumpMapCan.png");

  initTexture(4, m_woodTex, "images/woodDif.jpg");
  initTexture(5, m_woodSpec, "images/woodSpec.jpg");
  initTexture(6, m_woodNorm, "images/woodNorm.jpg");


//________________________________________________________________________________________________________________________________________//

  // we are creating a shader called gBuffer
  shader->createShaderProgram("gBuffer");
  // now we are going to create empty shaders for Frag and Vert
  shader->attachShader("gBufferVertex",ngl::ShaderType::VERTEX);
  shader->attachShader("gBufferFragment",ngl::ShaderType::FRAGMENT);
  // attach the source
  shader->loadShaderSource("gBufferVertex","shaders/gBufferVert.glsl");
  shader->loadShaderSource("gBufferFragment","shaders/gBufferFrag.glsl");
  // compile the shaders
  shader->compileShader("gBufferVertex");
  shader->compileShader("gBufferFragment");
  // add them to the program
  shader->attachShaderToProgram("gBuffer","gBufferVertex");
  shader->attachShaderToProgram("gBuffer","gBufferFragment");

  // now we have associated this data we can link the shader
  shader->linkProgramObject("gBuffer");

  shader->use("gBuffer");
  GLuint gBufferID = shader->getProgramID("gBuffer");
  glUniform1i(glGetUniformLocation(gBufferID, "gPosition"), 3);
  glUniform1i(glGetUniformLocation(gBufferID, "gNormal"), 4);
  glUniform1i(glGetUniformLocation(gBufferID, "gColour"), 5);
  shader->setUniform("labelMap", 2);

//________________________________________________________________________________________________________________________________________//

  // we are creating a shader called SSAO
  shader->createShaderProgram("SSAO");
  // now we are going to create empty shaders for Frag and Vert
  shader->attachShader("SSAOVertex",ngl::ShaderType::VERTEX);
  shader->attachShader("SSAOFragment",ngl::ShaderType::FRAGMENT);
  // attach the source
  shader->loadShaderSource("SSAOVertex","shaders/SSAOVert.glsl");
  shader->loadShaderSource("SSAOFragment","shaders/SSAOFrag.glsl");
  // compile the shaders
  shader->compileShader("SSAOVertex");
  shader->compileShader("SSAOFragment");
  // add them to the program
  shader->attachShaderToProgram("SSAO","SSAOVertex");
  shader->attachShaderToProgram("SSAO","SSAOFragment");

  // now we have associated this data we can link the shader
  shader->linkProgramObject("SSAO");
  shader->use("SSAO");
  m_SSAOID = shader->getProgramID("SSAO");
  glUniform1i(glGetUniformLocation(m_SSAOID, "gPosition"), 3);
  glUniform1i(glGetUniformLocation(m_SSAOID, "gNormal"), 4);
  glUniform1i(glGetUniformLocation(m_SSAOID, "texNoise"), 7);

//________________________________________________________________________________________________________________________________________//


  // we are creating a shader called SSAOBlur
  shader->createShaderProgram("SSAOBlur");
  // now we are going to create empty shaders for Frag and Vert
  shader->attachShader("SSAOBlurVertex",ngl::ShaderType::VERTEX);
  shader->attachShader("SSAOBlurFragment",ngl::ShaderType::FRAGMENT);
  // attach the source
  shader->loadShaderSource("SSAOBlurVertex","shaders/SSAOBlurVert.glsl");
  shader->loadShaderSource("SSAOBlurFragment","shaders/SSAOBlurFrag.glsl");
  // compile the shaders
  shader->compileShader("SSAOBlurVertex");
  shader->compileShader("SSAOBlurFragment");
  // add them to the program
  shader->attachShaderToProgram("SSAOBlur","SSAOBlurVertex");
  shader->attachShaderToProgram("SSAOBlur","SSAOBlurFragment");

  // now we have associated this data we can link the shader
  shader->linkProgramObject("SSAOBlur");

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
  shader->setUniform("bumpMap", 3);

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
  shader->setShaderParam3f("Light[1].Intensity", 0.6, 0.6, 1.0);
  shader->setShaderParam1f("Light[1].Linear", 0.35);
  shader->setShaderParam1f("Light[1].Quadratic", 0.44);
  shader->setShaderParam4f("Light[2].Position",4.0, 3.0, -1.0, 0.0);
  shader->setShaderParam3f("Light[2].La", 0.5, 0.5, 0.5);
  shader->setShaderParam3f("Light[2].Ld", 1.0, 0.1, 1.0);
  shader->setShaderParam3f("Light[2].Ls", 1.0, 1.0, 1.0);
  shader->setShaderParam3f("Light[2].Intensity", 0.8, 0.6, 0.6);
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
  CreateGBuffer();

  createSSAOBuffers();

  createSSAOKernelNoise();

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
  shader->setShaderParam1f("Light[0].Linear", 0.14);
  shader->setShaderParam1f("Light[0].Quadratic", 0.07);
  shader->setShaderParam4f("Light[1].Position",0.0, 2.0, 4.0, 1.0);
  shader->setShaderParam3f("Light[1].La", 0.5, 0.5, 0.5);
  shader->setShaderParam3f("Light[1].Ld", 0.1, 1.0, 1.0);
  shader->setShaderParam3f("Light[1].Ls", 1.0, 1.0, 1.0);
  shader->setShaderParam3f("Light[1].Intensity", 0.6, 0.6, 1.0);
  shader->setShaderParam1f("Light[1].Linear", 0.7);
  shader->setShaderParam1f("Light[1].Quadratic", 1.8);
  shader->setShaderParam4f("Light[2].Position",4.0, 3.0, -1.0, 0.0);
  shader->setShaderParam3f("Light[2].La", 0.5, 0.5, 0.5);
  shader->setShaderParam3f("Light[2].Ld", 1.0, 0.1, 1.0);
  shader->setShaderParam3f("Light[2].Ls", 1.0, 1.0, 1.0);
  shader->setShaderParam3f("Light[2].Intensity", 0.8, 0.6, 0.6);
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
    ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
    ngl::ShaderLib *shader=ngl::ShaderLib::instance();
// 1. Geometry Pass: render scene's geometry/color data into gbuffer
/*    glBindFramebuffer(GL_FRAMEBUFFER, m_gBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    loadMatrices("gBuffer");
    shader->use("gBuffer");
    m_mesh->draw();
    prim->draw("plane");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // 2. Create SSAO texture
    glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        shader->use("SSAO");
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_noiseTexture);
        // Send kernel + rotation
        for (GLuint i = 0; i < 64; ++i)
            glUniform3fv(glGetUniformLocation(m_SSAOID, ("samples[" + std::to_string(i) + "]").c_str()), 1, &m_ssaoKernel[i][0]);
  //      glUniformMatrix4fv(glGetUniformLocation(m_SSAOID, "projection"), 1, GL_FALSE, m_cam.getProjectionMatrix());
        shader->setShaderParamFromMat4("projection",m_cam.getProjectionMatrix());
        RenderQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 3. Blur SSAO texture to remove noise
    glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoBlurFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        shader->use("SSAOBlur");
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_ssaoColorBuffer);
        RenderQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);



// 4. Lighting Pass: traditional deferred Blinn-Phong lighting now with added screen-space ambient occlusion
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader->use(CanProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_gPosition);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_gNormal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_gColour);
    glActiveTexture(GL_TEXTURE3); // Add extra SSAO texture to lighting pass
    glBindTexture(GL_TEXTURE_2D, m_ssaoColorBufferBlur);
    // Also send light relevant uniforms
  //  glm::vec3 lightPosView = glm::vec3(camera.GetViewMatrix() * glm::vec4(lightPos, 1.0));
 //   glUniform3fv(glGetUniformLocation(m_CanID, "light.Position"), 1, &lightPosView[0]);
 //   glUniform3fv(glGetUniformLocation(m_CanID, "light.Color"), 1, &lightColor[0]);
    // Update attenuation parameters
  //  const GLfloat constant = 1.0; // Note that we don't send this to the shader, we assume it is always 1.0 (in our case)
 //   const GLfloat linear = 0.09;
 //   const GLfloat quadratic = 0.032;
 //   glUniform1f(glGetUniformLocation(shaderLightingPass.Program, "light.Linear"), linear);
 //   glUniform1f(glGetUniformLocation(shaderLightingPass.Program, "light.Quadratic"), quadratic);
 //   glUniform1i(glGetUniformLocation(shaderLightingPass.Program, "draw_mode"), draw_mode);
    m_transform.reset();
    m_transform.setPosition(0.0f,0.0f,0.0f);
    m_transform.setScale(0.4,0.4,0.4);
    loadMatrices(CanProgram);
    shader->use(CanProgram);
    shader->setShaderParam4f("Light[0].Position",m_lightPosition.m_x,m_lightPosition.m_y,m_lightPosition.m_z, 1.0);
  //  m_mesh->draw();
    RenderQuad();*/






//________________________________________________________________________________________________________________________________________//

  //----------------------------------------------------------------------------------------------------------------------
  // Pass 1 render our Depth texture to the FBO
  //----------------------------------------------------------------------------------------------------------------------
  // enable culling
  glEnable(GL_CULL_FACE);

  // bind the FBO and render offscreen to the texture
  glBindFramebuffer(GL_FRAMEBUFFER,m_ShadowfboID);
  // bind the texture object to 0 (off )
  glBindTexture(GL_TEXTURE_2D,0);
  // we need to render to the same size as the texture to avoid
  // distortions
  glViewport(0,0,TEXTURE_WIDTH,TEXTURE_HEIGHT);

  // Clear previous frame values
  glClear( GL_DEPTH_BUFFER_BIT);
  // as we are only rendering depth turn off the colour / alpha
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

  // render only the back faces so we don't get too much self shadowing
  glCullFace(GL_FRONT);
  // draw the scene from the POV of the light using the function we need
  drawScene(std::bind(&NGLScene::loadToLightPOVShader,this));

//________________________________________________________________________________________________________________________________________//

/*  // Bind the FBO to specify an alternative render target
  glBindFramebuffer(GL_FRAMEBUFFER, m_blurFBO);

  // Set up the viewport
  glViewport(0,0,width(),height());

  // Clear the screen (fill with our glClearColor)
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  m_transform.reset();
  m_transform.setPosition(0.0f,0.0f,0.0f);
  m_transform.setScale(0.4,0.4,0.4);
  loadMatrices(CanProgram);
  shader->use(CanProgram);
  shader->setShaderParam4f("Light[0].Position",m_lightPosition.m_x,m_lightPosition.m_y,m_lightPosition.m_z, 1.0);
  m_mesh->draw();


//________________________________________________________________________________________________________________________________________//

  // Unbind our FBO
  glBindFramebuffer(GL_FRAMEBUFFER,0);

  // Find the depth of field shader
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0,0,width(),height());

  // Now bind our rendered image which should be in the frame buffer for the next render pass
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, m_blurTexFBO);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, m_blurDepthFBO);

  shader->use("DOF");
  GLuint pid = shader->getProgramID("DOF");

  glUniform1i(glGetUniformLocation(pid, "colourTex"), 1);
  glUniform1i(glGetUniformLocation(pid, "depthTex"), 2);
  glUniform1f(glGetUniformLocation(pid, "focalDepth"), 1.0);
  glUniform1f(glGetUniformLocation(pid, "blurRadius"), 0.01);
  glUniform2f(glGetUniformLocation(pid, "windowSize"), width(), height());

  ngl::Mat4 MVP = m_cam.getVPMatrix();

  shader->setShaderParamFromMat4("MVP", MVP);

  prim->draw("plane");

*/


    //----------------------------------------------------------------------------------------------------------------------
  // Pass two use the texture
  // now we have created the texture for shadows render the scene properly
  //----------------------------------------------------------------------------------------------------------------------
  // go back to our normal framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER,0);
  // set the viewport to the screen dimensions
  glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());
  // enable colour rendering again (as we turned it off earlier)
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  // clear the screen
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(0.5f, 0.5f, 0.6f, 1.0f);

  // bind the shadow texture
  glBindTexture(GL_TEXTURE_2D,m_ShadowtextureID);


  // we need to generate the mip maps each time we bind
 // glGenerateMipmap(GL_TEXTURE_2D);

  // now only cull back faces
  glDisable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  // render our scene with the shadow shader
  drawScene(std::bind(&NGLScene::loadMatricesToShadowShader,this));



  //----------------------------------------------------------------------------------------------------------------------
  // this draws the debug texture on the quad
  //----------------------------------------------------------------------------------------------------------------------

 // glBindTexture(GL_TEXTURE_2D,m_textureID);
 // debugTexture(-0.6f,-1,0.6f,1);




  //----------------------------------------------------------------------------------------------------------------------
  // now draw the text
  //----------------------------------------------------------------------------------------------------------------------

  glDisable(GL_CULL_FACE);

  QString text=QString("Light Position [%1,%2,%3]")
                      .arg(m_lightPosition.m_x)
                      .arg(m_lightPosition.m_y)
                      .arg(m_lightPosition.m_z);

  m_text->setColour(1,1,1);
  m_text->renderText(250,20,text);
  text.sprintf("Y Position %0.2f",m_lightYPos);
  m_text->renderText(250,40,text);
  text.sprintf("X offset %0.2f",m_lightXoffset);
  m_text->renderText(250,60,text);
  text.sprintf("Z offset %0.2f",m_lightZoffset);
  m_text->renderText(250,80,text);

//________________________________________________________________________________________________________________________________________//

  //Draw the can

  m_transform.reset();
  m_transform.setPosition(0.0f,0.0f,0.0f);
  m_transform.setScale(0.4,0.4,0.4);
  loadMatrices(CanProgram);
  shader->use(CanProgram);
  shader->setShaderParam4f("Light[0].Position",m_lightPosition.m_x,m_lightPosition.m_y,m_lightPosition.m_z, 1.0);
  m_mesh->draw();
}

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//


void NGLScene::createShadowFBO()
{

  // Try to use a texture depth component
  glGenTextures(1, &m_ShadowtextureID);
  glBindTexture(GL_TEXTURE_2D, m_ShadowtextureID);
  //glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
  //glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);

  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

  glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

  glBindTexture(GL_TEXTURE_2D, 0);

  // create our FBO
  glGenFramebuffers(1, &m_ShadowfboID);
  glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowfboID);
  // disable the colour and read buffers as we only want depth
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);

  // attach our texture to the FBO

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D, m_ShadowtextureID, 0);

  // switch back to window-system-provided framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//

void NGLScene::createBlurFBO()
{
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
    glActiveTexture(GL_TEXTURE1);
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
    glBindTexture(GL_TEXTURE_2D, 0);

    // The depth buffer is rendered to a texture buffer too
    glGenTextures(1, &m_blurDepthFBO);
    glActiveTexture(GL_TEXTURE2);
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
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create the frame buffer
    glGenFramebuffers(1, &m_blurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_blurFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_blurTexFBO, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_blurDepthFBO, 0);

    // Set the fragment shader output targets (DEPTH_ATTACHMENT is done automatically)
    GLenum drawBufs[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, drawBufs);

    // Check it is ready to rock and roll
 //   CheckFrameBuffer();

    // Unbind the framebuffer to revert to default render pipeline
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
  // finally update the GLWindow and re-draw
  //if (isExposed())
    update();
}

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//

void NGLScene::updateLight()
{
  // change the light angle
  m_lightAngle+=0.02;
  m_lightPosition.set(m_lightXoffset*cos(m_lightAngle),m_lightYPos,m_lightXoffset*sin(m_lightAngle));
  // now set this value and load to the shader
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
  shader->use("Texture");
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
    // Set our active texture unit
    glActiveTexture(GL_TEXTURE0 + texUnit);

    // Load up the image using NGL routine
    ngl::Image img(filename);

    // Create storage for our new texture
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

    // Placing our environment map texture in texture unit 0
    glActiveTexture (GL_TEXTURE0);

    // Generate storage and a reference for our environment map texture
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

    // Set our cube map texture to on the shader so we can use it
    ngl::ShaderLib *shader=ngl::ShaderLib::instance();
    shader->use(CanProgram);
    shader->setUniform("envMap", 0);
    shader->use("Shadow");
    shader->setUniform("envMap", 0);
  //  shader->use("gBuffer");
  //  shader->setUniform("envMap", 0);

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
 //   GLint pid = shader->getProgramID(_program);
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
//    shader->setShaderParamFromMat3("V",V);
//    shader->setShaderParamFromMat3("P",P);
//    shader->setShaderParamFromMat3("M",M);
  //  shader->setShaderParam3f("LightPosition",m_lightPosition.m_x,m_lightPosition.m_y,m_lightPosition.m_z);
  //  shader->setShaderParam4f("inColour",1,1,1,1);
  //  shader->setShaderParamFromMat4("lights", P);


 //   shader->setShaderParam3f("LightPosition", m_lightPosition.m_x, m_lightPosition.m_y, m_lightPosition.m_z);

  //  glUniform

/*    ngl::Mat4 bias;

    bias.scale(0.5,0.5,0.5);
    bias.translate(0.5,0.5,0.5);

    ngl::Mat4 view=m_lightCamera.getViewMatrix();
    ngl::Mat4 proj=m_lightCamera.getProjectionMatrix();
    ngl::Mat4 model;

    ngl::Mat4 textureMatrix= model * view*proj * bias;
    shader->setShaderParamFromMat4("TextureMatrix",textureMatrix);*/
}

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//

/*void NGLScene::createNoiseTexture()
{
    int width = 128;
    int height = 128;
    noise::module::Perlin perlinNoise;
    // Base frequency for octave 1.
    perlinNoise.SetFrequency(4.0);
   // perlinNoise.SetPersistence(1.0);
    GLubyte *data = new GLubyte[ width * height * 4 ];
    double xRange = 1.0;
    double yRange = 1.0;
    double xFactor = xRange / width;
    double yFactor = yRange / height;

    for( int oct = 0; oct < 4; oct++ ) {
      perlinNoise.SetOctaveCount(oct+1);
      for( int i = 0; i < width; i++ ) {
        for( int j = 0 ; j < height; j++ ) {
          double x = xFactor * i;
          double y = yFactor * j;
          double z = 0.0;
          float val = 0.0f;
          double a, b, c, d;
          a = perlinNoise.GetValue(x       ,y       ,z);
          b = perlinNoise.GetValue(x+xRange,y       ,z);
          c = perlinNoise.GetValue(x       ,y+yRange,z);
          d = perlinNoise.GetValue(x+xRange,y+yRange,z);

          double xmix = 1.0 - x / xRange;
          double ymix = 1.0 - y / yRange;
          double x1 = glm::mix( a, b, xmix );
          double x2 = glm::mix( c, d, xmix );
          val = glm::mix(x1, x2, ymix );
          // Scale to roughly between 0 and 1
          val = (val + 1.0f) * 0.5f;
          // Clamp strictly between 0 and 1
          val = val> 1.0 ? 1.0 :val;
          val = val< 0.0 ? 0.0 :val;
                   // Store in texture
          data[((j * width + i) * 4) + oct] =
            (GLubyte) ( val * 255.0f );
            }
        }
    }
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,
                 GL_UNSIGNED_BYTE,data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    delete [] data;
}*/

//________________________________________________________________________________________________________________________________________//
//________________________________________________________________________________________________________________________________________//

void NGLScene::CreateGBuffer()
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
}

//________________________________________________________________________________________________________________________________________//

// RenderQuad() Renders a 1x1 quad in NDC, best used for framebuffer colour targets
// and post-processing effects.

void NGLScene::RenderQuad()
{
    if (m_quadVAO == 0)
    {
        GLfloat quadVertices[] = {
            // Positions        // Texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
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
