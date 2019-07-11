//
// Copyright (c) 2008-2018 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/Resource/ResourceEvents.h>
#include <Urho3D/DebugNew.h>
#include <Urho3D/Resource/XMLFile.h>

#include "SceneLoader.h"

#include "LoaderTools/ComponentExporter.h"
#include "commonComponents/CommonComponents.h"

#include "SampleComponents/PlayAnimation.h"
#include "game/gameComponents/GameComponents.h"
#include <Globals.h>
#include <Urho3D/Urho3DAll.h>
#include "CustomEvents.h"
#include "BlenderNetwork.h"

URHO3D_DEFINE_APPLICATION_MAIN(SceneLoader)

SceneLoader::SceneLoader(Context* context) :
    Sample(context)
    ,sceneName("Scene.xml")
    ,exportPath("./urho3d_components.json")
    ,currentCamId(0)
    ,updatedCamera(false)
    ,editorVisible_(false)

    ,screenshotInterval(2.0f)
    ,screenshotTimer(0.0f)
    ,automaticIntervallScreenshots(false)
    ,rtRenderRequested(false)
    ,surface(0)
{
    // register component exporter
    context->RegisterSubsystem(new Urho3DNodeTreeExporter(context));
    context->RegisterSubsystem(new BlenderNetwork(context));
    // register group instance component
    CommonComponents::RegisterComponents(context);
    GameComponents::RegisterComponents(context);


    engineParameters_[EP_WINDOW_RESIZABLE]=true;
}


void SceneLoader::Start()
{
    auto args = GetArguments();
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Globals::instance()->cache=cache;

    FileSystem* fs = context_->GetSubsystem<FileSystem>();

    URHO3D_LOGINFOF("[SceneLoader] Current dir:%s",fs->GetCurrentDir().CString());

    for (int i=0;i < args.Size(); i++){
        String arg = args[i];
        if (args[i]=="--workingdir" && (i+1)<args.Size()){
            String workpath = args[i+1];
            cache->AddResourceDir(workpath,0);
            additionalResourcePath = workpath;
            i++;
            URHO3D_LOGINFOF("[SceneLoader] added resourcepath:%s",workpath.CString());
        }
        else if (args[i]=="--scenename" && (i+1)<args.Size()){
            sceneName = args[i+1];
            i++;
            URHO3D_LOGINFOF("[SceneLoader] opening SCENE:%s",sceneName.CString());
        }
        else if (args[i]=="--runtimeflags" && (i+1)<args.Size()){
            String flags(args[i+1]);
            runtimeFlags = flags.Split(';');
            i++;
            URHO3D_LOGINFOF("[SceneLoader] runtime-flags: %s",flags.CString());
        }
        else if (args[i]=="--componentexport" && (i+1)<args.Size()){
            exportPath = args[i+1];
            i++;
            URHO3D_LOGINFOF("[SceneLoader] exportPath: %s",exportPath.CString());
        }
        else if (args[i]=="--customui" && (i+1)<args.Size()){
            customUI = args[i+1];
            i++;
            URHO3D_LOGINFOF("[SceneLoader] customui: %s",customUI.CString());
        }

    }
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    bool foundScene = CreateScene();
    if (!foundScene)
        return;

    // Create the UI content
    CreateUI();

    // Setup the viewport for displaying the scene
    SetupViewport();

    InitRenderTarget();

    // Subscribe to global events for camera movement
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);

    ExportComponents(exportPath);


}

void SceneLoader::Stop()
{
    BlenderNetwork* blenderNetwork = GetSubsystem<BlenderNetwork>();
    if (blenderNetwork){
        blenderNetwork->Close();
    }
}

void SceneLoader::ExportComponents(const String& outputPath)
{
    Urho3DNodeTreeExporter* exporter = GetSubsystem<Urho3DNodeTreeExporter>();
    // set whitelist-mode to tell the exporter what components to include for export
    exporter->SetExportMode(Urho3DNodeTreeExporter::WhiteList);

    // include all Components that inherit from LogicComponent
    exporter->AddSuperComponentHashToFilterList(LogicComponent::GetTypeStatic());
    // explicitly export those components
    exporter->AddComponentHashToFilterList(Light::GetTypeStatic());
    exporter->AddComponentHashToFilterList(RigidBody::GetTypeStatic());
    exporter->AddComponentHashToFilterList(CollisionShape::GetTypeStatic());
    exporter->AddComponentHashToFilterList(Navigable::GetTypeStatic());
    exporter->AddComponentHashToFilterList(NavArea::GetTypeStatic());
    exporter->AddComponentHashToFilterList(GroupInstance::GetTypeStatic());
    exporter->AddComponentHashToFilterList(PlayAnimation::GetTypeStatic());
    exporter->AddComponentHashToFilterList(NavigationMesh::GetTypeStatic());
    exporter->AddComponentHashToFilterList(Octree::GetTypeStatic());
    exporter->AddComponentHashToFilterList(PhysicsWorld::GetTypeStatic());
    exporter->AddComponentHashToFilterList(DebugRenderer::GetTypeStatic());
    exporter->AddComponentHashToFilterList(Zone::GetTypeStatic());
    exporter->AddComponentHashToFilterList(AnimationController::GetTypeStatic());

    exporter->AddCustomUIFile("./customui.py");
    exporter->AddMaterialFolder("Materials");
    exporter->AddTechniqueFolder("Techniques");
    exporter->AddTextureFolder("Textures");
    exporter->AddModelFolder("Models");
    exporter->AddAnimationFolder("Models");
    if (!customUI.Empty()){
        exporter->AddCustomUIFile(customUI);
    }
    exporter->Export(outputPath);
}

bool SceneLoader::CreateScene()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);
    SharedPtr<File> file = cache->GetFile("Scenes/"+sceneName);
    if (file.Null()){
        URHO3D_LOGERROR("SceneLoader could not find 'Scenes/Scene.xml' in its resource-path");
        engine_->Exit();
        return false;
    }
    cache->SetAutoReloadResources(true);
    scene_->LoadXML(*file);
    Globals::instance()->scene=scene_;

    // Create the camera (not included in the scene file)
    cameraNode_ = scene_->CreateChild("Camera");
    Camera* camera = cameraNode_->CreateComponent<Camera>();
    cameraNode_->SetPosition(Vector3(0.0f, 2.0f, -10.0f));
    Globals::instance()->camera=camera;

    // set a light
    // TODO: Use only the light provided by scene (once this is exported correctly)
    PODVector<Light*> sceneLights;
    scene_->GetComponents<Light>(sceneLights,true);

    if (sceneLights.Size()==0){
        Node* lightNode = scene_->CreateChild("DirectionalLight");

        //lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f)); // The direction vector does not need to be normalized
        lightNode->SetRotation(Quaternion(18.0f,55.0f,-17.0f));
        Light* light = lightNode->CreateComponent<Light>();
        light->SetLightType(LIGHT_DIRECTIONAL);
        light->SetCastShadows(true);
        light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
        light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
        light->SetSpecularIntensity(0.5f);
        //light->SetColor(Color::RED);
    }



    PODVector<Node*> dest;
    if (scene_->GetNodesWithTag(dest,"setmesh")){
        for (Node* node : dest){
            CollisionShape* shape = node->GetComponent<CollisionShape>();
            if (shape && shape->GetShapeType() == SHAPE_TRIANGLEMESH){
                StaticModel* model = node->GetComponent<StaticModel>();
                shape->SetModel(model->GetModel());
            }
        }
    }
    File saveFile(context_, "./scene.write.xml",FILE_WRITE);
    scene_->SaveXML(saveFile);


    return true;
}



void SceneLoader::ReloadScene()
{
    updatedCamera = false;
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    SharedPtr<File> file = cache->GetFile("Scenes/Scene.xml");
    if (file.Null()){
        URHO3D_LOGERROR("SceneLoader could not find 'Scenes/Scene.xml' in its resource-path");
        engine_->Exit();
    }
    scene_->LoadXML(*file);

    // check if the scene has a light
    PODVector<Light*> sceneLights;
    scene_->GetComponents<Light>(sceneLights,true);

    if (sceneLights.Size()==0){
        Node* lightNode = scene_->CreateChild("DirectionalLight");
        lightNode->SetRotation(Quaternion(18.0f,55.0f,-17.0f));

        //lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f)); // The direction vector does not need to be normalized
        Light* light = lightNode->CreateComponent<Light>();
        light->SetLightType(LIGHT_DIRECTIONAL);
        light->SetCastShadows(true);
        light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
        light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
        light->SetSpecularIntensity(0.5f);
     //   light->SetColor(Color::RED);
        light->SetBrightness(2.5f);
    }

    UpdateCameras();

    File saveFile(context_, "./scene.write.xml",FILE_WRITE);
    scene_->SaveXML(saveFile);


}

void SceneLoader::UpdateCameras()
{
    cameras.Clear();
    cameras.Push(Globals::instance()->camera);
    scene_->GetComponents<Camera>(cameras,true);
}

void SceneLoader::CreateUI()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    // Set up global UI style into the root UI element
    auto* style = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    ui->GetRoot()->SetDefaultStyle(style);

    // Create a Cursor UI element because we want to be able to hide and show it at will. When hidden, the mouse cursor will
    // control the camera, and when visible, it will interact with the UI
    SharedPtr<Cursor> cursor(new Cursor(context_));
    cursor->SetStyleAuto();
    ui->SetCursor(cursor);
    // Set starting position of the cursor at the rendering window center
    Graphics* graphics = GetSubsystem<Graphics>();
    cursor->SetPosition(graphics->GetWidth() / 2, graphics->GetHeight() / 2);
    // Load UI content prepared in the editor and add to the UI hierarchy
}

void SceneLoader::SetupViewport()
{
    auto* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);

}

void SceneLoader::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for camera motion
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(SceneLoader, HandleUpdate));
    using namespace FileChanged;
    SubscribeToEvent(E_FILECHANGED, URHO3D_HANDLER(SceneLoader, HandleFileChanged));
    SubscribeToEvent(E_ENDALLVIEWSRENDER, URHO3D_HANDLER(SceneLoader, HandleAfterRender));
    using namespace BlenderConnect;
    SubscribeToEvent(E_BLENDER_MSG, URHO3D_HANDLER(SceneLoader,HandleBlenderMSG));
}

void SceneLoader::MoveCamera(float timeStep)
{
    // Right mouse button controls mouse cursor visibility: hide when pressed
    auto* ui = GetSubsystem<UI>();
    auto* input = GetSubsystem<Input>();
    ui->GetCursor()->SetVisible(!input->GetMouseButtonDown(MOUSEB_RIGHT));

    // Do not move if the UI has a focused element
    if (ui->GetFocusElement())
        return;

    // Movement speed as world units per second
    const float MOVE_SPEED = 20.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    // Only move the camera when the cursor is hidden
    if (!ui->GetCursor()->IsVisible())
    {
        IntVector2 mouseMove = input->GetMouseMove();
        yaw_ += MOUSE_SENSITIVITY * mouseMove.x_ * 0.75f;
        pitch_ += MOUSE_SENSITIVITY * mouseMove.y_ * 0.75f;
        pitch_ = Clamp(pitch_, -90.0f, 90.0f);

        // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
        cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
    }

    float moveSpeed = input->GetMouseButtonDown(MOUSEB_RIGHT)?0.25f:1.0f;

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep *moveSpeed);
    if (input->GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep*moveSpeed);
    if (input->GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep*moveSpeed);
    if (input->GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep*moveSpeed);
}

void SceneLoader::HandleFileChanged(StringHash eventType, VariantMap& eventData)
{
    using namespace FileChanged;
    String filename = eventData[P_FILENAME].GetString();
    String resName = eventData[P_RESOURCENAME].GetString();

    if (resName=="Scenes/Scene.xml"){
        ReloadScene();
    }
//    else if (resName=="req2engine.json"){
//        JSONFile json(context_);
//        if (json.LoadFile(filename)){
//            HandleRequestFromBlender(json.GetRoot().GetObject());
//        }
//    }

    //URHO3D_LOGINFOF("File Changed: fn:%s resname:%s",filename.CString(),resName.CString());
}


void SceneLoader::HandleBlenderMSG(StringHash eventType, VariantMap &eventData)
{
    using namespace BlenderConnect;
    auto topic = eventData[P_TOPIC].GetString();
    JSONObject data  = *static_cast<JSONObject*>(eventData[P_DATA].GetVoidPtr());
    HandleRequestFromBlender(data);
    int a=0;
}

void SceneLoader::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    if (currentCamId==0){
        MoveCamera(timeStep);
    }

    Input* input = GetSubsystem<Input>();
    if (input->GetKeyPress(KEY_C)){
        URHO3D_LOGINFO("CAMERA CHANGE");
        currentCamId++;
        if (currentCamId>=cameras.Size()){
            currentCamId = 0;
        }
        if (currentCamId == 0){
            Sample::InitMouseMode(MM_FREE);
        } else {
            Sample::InitMouseMode(MM_RELATIVE);
        }
        Renderer* renderer = GetSubsystem<Renderer>();
        Viewport* viewport = renderer->GetViewport(0);
        viewport->SetCamera(cameras[currentCamId]);
    }
    else if (input->GetKeyPress(KEY_F12)){
        if (!editorVisible_){
            InitEditor();
            editorVisible_=true;
        }
    }
    else if (input->GetKeyPress(KEY_0)){
        BlenderNetwork* bN = GetSubsystem<BlenderNetwork>();
    }

    if (!updatedCamera){
        UpdateCameras();
        updatedCamera = false;
    }

    if (rtRenderRequested){
        if (screenshotTimer > 0){
            screenshotTimer-=timeStep;
        }
    }
}


/*void SceneLoader::CreateScreenshot()
{
    if (additionalResourcePath=="") return;

    Graphics* graphics = GetSubsystem<Graphics>();
    Image screenshot(context_);
    graphics->TakeScreenShot(screenshot);
    // Here we save in the Data folder with date and time appended
//    screenshot.SavePNG(GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Screenshot_" +
//        Time::GetTimeStamp().Replaced(':', '_').Replaced('.', '_').Replaced(' ', '_') + ".png");
    String screenshotPath = additionalResourcePath+"/Screenshot.png";
    screenshot.SavePNG(screenshotPath);
   // URHO3D_LOGINFOF("Save screenshot to %s",screenshotPath.CString());
}

Vector4 JSON2Vec4(const JSONObject& v){
    return Vector4(v["x"]->GetFloat(),v["y"]->GetFloat(),v["z"]->GetFloat(),v["w"]->GetFloat());
}
Vector3 JSON2Vec3(const JSONObject& v){
    return Vector3(v["x"]->GetFloat(),v["y"]->GetFloat(),v["z"]->GetFloat());
}
*/
//void SceneLoader::HandleRequestFromBlender(const JSONObject &json)
//{


//    if (json.Contains("view_matrix")){

//        int width = json["view_width"]->GetInt();
//        int height = json["view_height"]->GetInt();

//        if (width!=rtTexture->GetWidth() || height!=rtTexture->GetHeight()){
//            rtTexture->SetSize(width, height, Graphics::GetRGBAFormat(), TEXTURE_RENDERTARGET);
//            surface = rtTexture->GetRenderSurface();
//            SharedPtr<Viewport> rttViewport(new Viewport(context_, scene_, blenderViewportCamera));
//            surface->SetViewport(0, rttViewport);
//            surface->SetUpdateMode(SURFACE_MANUALUPDATE);
//        }



//        JSONArray matrix = json["perspective_matrix"]->GetArray();
//        Vector4 v1 = JSON2Vec4(matrix[0].GetObject());
//        Vector4 v2 = JSON2Vec4(matrix[1].GetObject());
//        Vector4 v3 = JSON2Vec4(matrix[2].GetObject());
//        Vector4 v4 = JSON2Vec4(matrix[3].GetObject());


//        Matrix4 pmat(v1.x_,v1.y_,v1.z_,v1.w_,
//                    v2.x_,v2.y_,v2.z_,v2.w_,
//                    v3.x_,v3.y_,v3.z_,v3.w_,
//                    v4.x_,v4.y_,v4.z_,v4.w_);


//        JSONArray matrix2 = json["view_matrix"]->GetArray();
//        Vector4 v21 = JSON2Vec4(matrix2[0].GetObject());
//        Vector4 v22 = JSON2Vec4(matrix2[1].GetObject());
//        Vector4 v23 = JSON2Vec4(matrix2[2].GetObject());
//        Vector4 v24 = JSON2Vec4(matrix2[3].GetObject());

//        Matrix4 vmat(v21.x_,v21.y_,v21.z_,v21.w_,
//                    v22.x_,v22.y_,v22.z_,v22.w_,
//                    v23.x_,v23.y_,v23.z_,v23.w_,
//                    v24.x_,v24.y_,v24.z_,v24.w_);

//    /*    Matrix4 vmat(v1.x_,v1.z_,v1.y_,v1.w_,
//                    v3.x_,v3.z_,v3.y_,v3.w_,
//                    v2.x_,v2.z_,v2.y_,v2.w_,
//                    v4.x_,v4.z_,v4.y_,v4.w_);*/


//        Renderer* renderer = GetSubsystem<Renderer>();
//        Viewport* viewport = renderer->GetViewport(0);
//        Camera* c = viewport->GetCamera();
//        c->SetFarClip(1000.0f);
//        auto t = vmat.Translation();

//        auto r = vmat.Rotation().EulerAngles();
//        URHO3D_LOGINFOF("ROT:%s",r.ToString().CString());
//        auto s = vmat.Scale();

//        blenderViewportCameraNode->SetPosition(Vector3(0,0,0));
//        blenderViewportCameraNode->SetRotation(Quaternion(r.x_+90,r.z_-90,0));
//        blenderViewportCameraNode->Translate(Vector3(-t.x_,-t.y_,t.z_));

//        rtRenderRequested = true;
//        surface->QueueUpdate();
//    }
//}

//void SceneLoader::HandleRequestFromEngineToBlender()
//{
//    FileSystem* f = GetSubsystem<FileSystem>();
//    f->Delete(additionalResourcePath+"/req2engine.json");

//    if (additionalResourcePath!=""){
//        JSONFile* file = new JSONFile(context_);
//        JSONObject responseToBlender;
//        responseToBlender["action"]="reload_screenshot";
//        file->GetRoot()=responseToBlender;
//        file->SaveFile(additionalResourcePath+"/runtime2blender.json");
//    }
//}

void SceneLoader::InitRenderTarget()
{
    blenderViewportCameraNode=scene_->CreateChild("blenderCamera");
    blenderViewportCamera = blenderViewportCameraNode->CreateComponent<Camera>();


    // Create a renderable texture (1024x768, RGB format), enable bilinear filtering on it
    rtTexture = new Texture2D(context_);
    rtTexture->SetSize(1024, 768, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
    rtTexture->SetFilterMode(FILTER_BILINEAR);

    // Get the texture's RenderSurface object (exists when the texture has been created in rendertarget mode)
    // and define the viewport for rendering the second scene, similarly as how backbuffer viewports are defined
    // to the Renderer subsystem. By default the texture viewport will be updated when the texture is visible
    // in the main view
    surface = rtTexture->GetRenderSurface();
    SharedPtr<Viewport> rttViewport(new Viewport(context_, scene_, blenderViewportCamera));
    surface->SetViewport(0, rttViewport);
    surface->SetUpdateMode(SURFACE_MANUALUPDATE);
}

void SceneLoader::InitEditor()
{
    if (!context_->GetSubsystem<Script>()){
        // Instantiate and register the AngelScript subsystem
        context_->RegisterSubsystem(new Script(context_));
        context_->RegisterSubsystem(new LuaScript(context_));

        // Hold a shared pointer to the script file to make sure it is not unloaded during runtime
        scriptFile_ = GetSubsystem<ResourceCache>()->GetResource<ScriptFile>("Scripts/Editor.as");
    }

    /// \hack If we are running the editor, also instantiate Lua subsystem to enable editing Lua ScriptInstances
    // If script loading is successful, proceed to main loop
    if (scriptFile_ && scriptFile_->Execute("void Start()"))
    {
        // Subscribe to script's reload event to allow live-reload of the application
        SubscribeToEvent(scriptFile_, E_RELOADSTARTED, URHO3D_HANDLER(SceneLoader, HandleScriptReloadStarted));
        SubscribeToEvent(scriptFile_, E_RELOADFINISHED, URHO3D_HANDLER(SceneLoader, HandleScriptReloadFinished));
        SubscribeToEvent(scriptFile_, E_RELOADFAILED, URHO3D_HANDLER(SceneLoader, HandleScriptReloadFailed));
        return;
    }
}


void SceneLoader::HandleScriptReloadStarted(StringHash eventType, VariantMap& eventData)
{
#ifdef URHO3D_ANGELSCRIPT
    if (scriptFile_->GetFunction("void Stop()"))
        scriptFile_->Execute("void Stop()");
#endif
}

void SceneLoader::HandleScriptReloadFinished(StringHash eventType, VariantMap& eventData)
{
#ifdef URHO3D_ANGELSCRIPT
    // Restart the script application after reload
    if (!scriptFile_->Execute("void Start()"))
    {
        scriptFile_.Reset();
        ErrorExit();
    }
#endif
}

void SceneLoader::HandleScriptReloadFailed(StringHash eventType, VariantMap& eventData)
{
#ifdef URHO3D_ANGELSCRIPT
    scriptFile_.Reset();
    ErrorExit();
#endif
}


Vector4 JSON2Vec4(const JSONObject& v){
    return Vector4(v["x"]->GetFloat(),v["y"]->GetFloat(),v["z"]->GetFloat(),v["w"]->GetFloat());
}
Vector3 JSON2Vec3(const JSONObject& v){
    return Vector3(v["x"]->GetFloat(),v["y"]->GetFloat(),v["z"]->GetFloat());
}


void SceneLoader::HandleRequestFromBlender(const JSONObject &json)
{


    if (json.Contains("view_matrix")){

        int width = json["view_width"]->GetInt();
        int height = json["view_height"]->GetInt();

      /*  if (width!=rtTexture->GetWidth() || height!=rtTexture->GetHeight()){
            rtTexture->SetSize(width, height, Graphics::GetRGBAFormat(), TEXTURE_RENDERTARGET);
            surface = rtTexture->GetRenderSurface();
            SharedPtr<Viewport> rttViewport(new Viewport(context_, scene_, blenderViewportCamera));
            surface->SetViewport(0, rttViewport);
            surface->SetUpdateMode(SURFACE_MANUALUPDATE);
        }
*/


        JSONArray matrix = json["perspective_matrix"]->GetArray();
        Vector4 v1 = JSON2Vec4(matrix[0].GetObject());
        Vector4 v2 = JSON2Vec4(matrix[1].GetObject());
        Vector4 v3 = JSON2Vec4(matrix[2].GetObject());
        Vector4 v4 = JSON2Vec4(matrix[3].GetObject());


        Matrix4 pmat(v1.x_,v1.y_,v1.z_,v1.w_,
                    v2.x_,v2.y_,v2.z_,v2.w_,
                    v3.x_,v3.y_,v3.z_,v3.w_,
                    v4.x_,v4.y_,v4.z_,v4.w_);


        JSONArray matrix2 = json["view_matrix"]->GetArray();
        Vector4 v21 = JSON2Vec4(matrix2[0].GetObject());
        Vector4 v22 = JSON2Vec4(matrix2[1].GetObject());
        Vector4 v23 = JSON2Vec4(matrix2[2].GetObject());
        Vector4 v24 = JSON2Vec4(matrix2[3].GetObject());

        Matrix4 vmat(v21.x_,v21.y_,v21.z_,v21.w_,
                    v22.x_,v22.y_,v22.z_,v22.w_,
                    v23.x_,v23.y_,v23.z_,v23.w_,
                    v24.x_,v24.y_,v24.z_,v24.w_);

    /*    Matrix4 vmat(v1.x_,v1.z_,v1.y_,v1.w_,
                    v3.x_,v3.z_,v3.y_,v3.w_,
                    v2.x_,v2.z_,v2.y_,v2.w_,
                    v4.x_,v4.z_,v4.y_,v4.w_);*/


        Renderer* renderer = GetSubsystem<Renderer>();
        Viewport* viewport = renderer->GetViewport(0);
        Camera* c = viewport->GetCamera();
        c->SetFarClip(1000.0f);
        auto t = vmat.Translation();

        auto r = vmat.Rotation().EulerAngles();
        URHO3D_LOGINFOF("ROT:%s",r.ToString().CString());
        auto s = vmat.Scale();

        blenderViewportCameraNode->SetPosition(Vector3(0,0,0));
        blenderViewportCameraNode->SetRotation(Quaternion(r.x_+90,r.z_-90,0));
        blenderViewportCameraNode->Translate(Vector3(-t.x_,-t.y_,t.z_));

        rtRenderRequested = true;
        surface->QueueUpdate();
    }
}


void SceneLoader::HandleAfterRender(StringHash eventType, VariantMap& eventData)
{
    if (rtRenderRequested && screenshotTimer <= 0 ){
        screenshotTimer = screenshotInterval;
        rtRenderRequested=false;

        URHO3D_LOGINFO("AFTER RENDER");
        Image* _pImage = new Image(context_);
        _pImage->SetSize(rtTexture->GetWidth(), rtTexture->GetHeight(), 4);

        unsigned char* _ImageData = new unsigned char[rtTexture->GetDataSize(rtTexture->GetWidth(), rtTexture->GetHeight())];
        rtTexture->GetData(0, _ImageData);

        _pImage->SetData(_ImageData);

        _pImage->SavePNG(additionalResourcePath+"/Screenshot.png");

        delete[] _ImageData;

        BlenderNetwork* bN = GetSubsystem<BlenderNetwork>();
        bN->Send("blender","renderready");

        //HandleRequestFromEngineToBlender();
    }

}
