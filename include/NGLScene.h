#ifndef NGLSCENE_H_
#define NGLSCENE_H_
#include <ngl/Camera.h>
#include <ngl/Colour.h>
#include <ngl/Light.h>
#include <ngl/Transformation.h>
#include <ngl/Text.h>
#include "WindowParams.h"
#include <QOpenGLWindow>
#include <memory>
#include <ngl/Obj.h>
#include <glm/vec3.hpp>
//----------------------------------------------------------------------------------------------------------------------
/// @file NGLScene.h
/// @brief this class inherits from the Qt OpenGLWindow and allows us to use NGL to draw OpenGL
/// @author Jonathan Macey
/// @version 1.0
/// @date 10/9/13
/// Revision History :
/// This is an initial version used for the new NGL6 / Qt 5 demos
/// @class NGLScene
/// @brief our main glwindow widget for NGL applications all drawing elements are
/// put in this file
//----------------------------------------------------------------------------------------------------------------------

class NGLScene : public QOpenGLWindow
{
  public:
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief ctor for our NGL drawing class
    /// @param [in] parent the parent window to the class
    //----------------------------------------------------------------------------------------------------------------------
    NGLScene();
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief dtor must close down ngl and release OpenGL resources
    //----------------------------------------------------------------------------------------------------------------------
    ~NGLScene();
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief the initialize class is called once when the window is created and we have a valid GL context
    /// use this to setup any default GL stuff
    //----------------------------------------------------------------------------------------------------------------------
    void initializeGL();
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief this is called everytime we want to draw the scene
    //----------------------------------------------------------------------------------------------------------------------
    void paintGL();
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief this is called everytime we want resize
    //----------------------------------------------------------------------------------------------------------------------
    void resizeGL(int _w, int _h);
private:
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief the windows params such as mouse and rotations etc
    //----------------------------------------------------------------------------------------------------------------------
    WinParams m_win;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief used to store the global mouse transforms
    //----------------------------------------------------------------------------------------------------------------------
    ngl::Mat4 m_mouseGlobalTX;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief Our Camera
    //----------------------------------------------------------------------------------------------------------------------
    ngl::Camera m_cam;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief camera from light view
    //----------------------------------------------------------------------------------------------------------------------
    ngl::Camera m_lightCamera;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief transformation stack for the gl transformations etc
    //----------------------------------------------------------------------------------------------------------------------
    ngl::Transformation m_transform;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief the model position for mouse movement
    //----------------------------------------------------------------------------------------------------------------------
    ngl::Vec3 m_modelPos;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief a simple light use to illuminate the screen
    //----------------------------------------------------------------------------------------------------------------------
    ngl::Light *m_light;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief timer id for the animation timer
    //----------------------------------------------------------------------------------------------------------------------
    int m_lightTimer;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief flag to indicate if were animating the light
    //----------------------------------------------------------------------------------------------------------------------
    bool m_animate;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief the angle of the light updated when the timer triggers to make it rotate
    //----------------------------------------------------------------------------------------------------------------------
    ngl::Real m_lightAngle;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief the position of the light used to create the shadow map
    //----------------------------------------------------------------------------------------------------------------------
    ngl::Vec3 m_lightPosition;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief our texture id used by the FBO
    //----------------------------------------------------------------------------------------------------------------------
    GLuint m_ShadowtextureID;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief our FBO id used by the FBO
    //----------------------------------------------------------------------------------------------------------------------
    GLuint m_ShadowfboID;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief y pos of the light
    //----------------------------------------------------------------------------------------------------------------------
    GLfloat m_lightYPos;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief x offset of the light
    //----------------------------------------------------------------------------------------------------------------------
    GLfloat m_lightXoffset;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief z offset of the light
    //----------------------------------------------------------------------------------------------------------------------
    GLfloat m_lightZoffset;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief text for debug
    //----------------------------------------------------------------------------------------------------------------------
    std::unique_ptr<ngl::Text> m_text;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief a method to update the light
    //----------------------------------------------------------------------------------------------------------------------
    void updateLight();
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief draw our scene passing in the shader to use
    /// @param[in] _shader the name of the shader to use when drawing
    /// @param[in] _shaderFunc the function to load values to the shader
    //----------------------------------------------------------------------------------------------------------------------
    void drawScene(std::function<void()> _shaderFunc);
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief load all the transform values to the shader
    /// @param[in] _tx the current transform to load
    //----------------------------------------------------------------------------------------------------------------------
    void loadMatricesToShadowShader();
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief load all the transform values to the shader from light POV
    /// @param[in] _tx the current transform to load
    //----------------------------------------------------------------------------------------------------------------------
    void loadToLightPOVShader();
    void debugTexture(float _t, float _b, float _l, float _r);
    void createShadowFBO();
    void createBlurFBO();
    inline void toggleAnimation(){m_animate ^=true;}
    inline void changeLightYPos(float _dy){m_lightYPos+=_dy;}
    inline void changeLightZOffset(float _dz){m_lightZoffset+=_dz;}
    inline void changeLightXOffset(float _dx){m_lightXoffset+=_dx;}

    //----------------------------------------------------------------------------------------------------------------------
    /// @brief method to load transform matrices to the shader
    //----------------------------------------------------------------------------------------------------------------------
    void loadMatricesToShader();
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief Qt Event called when a key is pressed
    /// @param [in] _event the Qt event to query for size etc
    //----------------------------------------------------------------------------------------------------------------------
    void keyPressEvent(QKeyEvent *_event);
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief this method is called every time a mouse is moved
    /// @param _event the Qt Event structure
    //----------------------------------------------------------------------------------------------------------------------
    void mouseMoveEvent (QMouseEvent * _event );
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief this method is called everytime the mouse button is pressed
    /// inherited from QObject and overridden here.
    /// @param _event the Qt Event structure
    //----------------------------------------------------------------------------------------------------------------------
    void mousePressEvent ( QMouseEvent *_event);
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief this method is called everytime the mouse button is released
    /// inherited from QObject and overridden here.
    /// @param _event the Qt Event structure
    //----------------------------------------------------------------------------------------------------------------------
    void mouseReleaseEvent ( QMouseEvent *_event );

    //----------------------------------------------------------------------------------------------------------------------
    /// @brief this method is called everytime the mouse wheel is moved
    /// inherited from QObject and overridden here.
    /// @param _event the Qt Event structure
    //----------------------------------------------------------------------------------------------------------------------
    void wheelEvent( QWheelEvent *_event);
    void timerEvent(QTimerEvent *);

    /// Initialise the entire environment map
    void initEnvironment();

    /// Utility function for loading up a 2D texture
    void initTexture(const GLuint&, GLuint &, const char *);

    /// Initialise a single side of the environment map
    void initEnvironmentSide(GLenum /*target*/, const char* /*filename*/);

    void loadMatrices(const std::string _program);

    void createNoiseTexture();

    void CreateGBuffer();

    void createSSAOBuffers();

    void createSSAOKernelNoise();

    void RenderQuad();

    GLfloat lerp(GLfloat a, GLfloat b, GLfloat f);



    /// A unique pointer storing our mesh object
    std::unique_ptr<ngl::Obj> m_mesh;

    ///For the light
   // std::unique_ptr<ngl::Light> m_light;

    /// The ID of can textures
    GLuint m_envTex, m_glossMapTex, m_labelTex, m_bumpTex, m_textureMap;
    /// The ID of ground textures
    GLuint m_woodTex, m_woodSpec, m_woodNorm;

    GLuint m_gBuffer, m_blurFBO, m_blurTexFBO, m_blurDepthFBO, m_ssaoFBO, m_ssaoBlurFBO, m_ssaoColorBuffer, m_ssaoColorBufferBlur;

    GLuint m_gPosition, m_gNormal, m_gColour,m_noiseTexture;

    std::vector<glm::vec3> m_ssaoKernel;

    GLuint m_SSAOID, m_CanID;

    GLuint m_quadVAO;
    GLuint m_quadVBO;

    unsigned int m_pingpongFBO, m_pingpongColourBuffers;


};



#endif
