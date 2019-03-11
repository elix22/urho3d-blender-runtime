#include "ComponentExporter.h"

#include <Urho3D/Urho3DAll.h>
#include "base64.h"
#include <Urho3D/Container/Sort.h>


Urho3DNodeTreeExporter::Urho3DNodeTreeExporter(Context* context, ExportMode exportMode)
    : Object(context),
      m_exportMode(exportMode)
{
}

void Urho3DNodeTreeExporter::AddComponentHashToFilterList(const StringHash& componentHash)
{
    m_listOfComponents.Insert(componentHash);
}

void Urho3DNodeTreeExporter::AddSuperComponentHashToFilterList(const StringHash& superComponentHash)
{
    m_listOfSuperClasses.Insert(superComponentHash);
}

bool Urho3DNodeTreeExporter::CheckSuperTypes(const TypeInfo* type)
{
    for (auto superType : m_listOfSuperClasses){
        if (!type->IsTypeOf(superType)){
            return false;
        }
    }
    return true;
}


bool CompareString(const String& a,const String& b){
    return a < b;
}

JSONObject Urho3DNodeTreeExporter::ExportMaterials()
{
    const String treeID="urho3dmaterials";

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    FileSystem* fs = GetSubsystem<FileSystem>();

    Vector<String> materialFiles;
    Vector<String> techniqueFiles;
    Vector<String> textureFiles;

    for (String resDir : cache->GetResourceDirs()){
        Vector<String> dirFiles;
        for (String path : m_materialFolders){
            String dir = resDir+path;
            fs->ScanDir(dirFiles,dir,"*.xml",SCAN_FILES,true);
            for (String foundMaterial : dirFiles){
                auto materialResourceName = path+"/"+foundMaterial;
                Material* material = cache->GetResource<Material>(materialResourceName);
                if (material){
                    materialFiles.Push(materialResourceName);
                }
            }
        }


        // grab techniques from the specified technique folders
        for (String path : m_techniqueFolders){
            String dir = resDir+path;
            fs->ScanDir(dirFiles,dir,"*.xml",SCAN_FILES,true);
            for (String foundTechnique : dirFiles){
                auto techiqueResourceName = path+"/"+foundTechnique;
                Technique* technique = cache->GetResource<Technique>(techiqueResourceName);
                if (technique){
                    techniqueFiles.Push(techiqueResourceName);
                }
            }
        }


        // grab textures from the specified technique folders
        for (String path : m_textureFolders){
            String dir = resDir+path;
            fs->ScanDir(dirFiles,dir,"*.jpg",SCAN_FILES,true);
            for (String foundTexture : dirFiles){
                auto textureResourceName = path+"/"+foundTexture;
                textureFiles.Push(textureResourceName);
//                Texture* texture = cache->GetResource<Texture>(textureResourceName);
//                if (texture){
//                    textureFiles.Push(textureResourceName);
//                }
            }
            fs->ScanDir(dirFiles,dir,"*.png",SCAN_FILES,true);
            for (String foundTexture : dirFiles){
                auto textureResourceName = path+"/"+foundTexture;
                textureFiles.Push(textureResourceName);
//                Texture* texture = cache->GetResource<Texture>(textureResourceName);
//                if (texture){
//                    textureFiles.Push(textureResourceName);
//                }
            }
            fs->ScanDir(dirFiles,dir,"*.dds",SCAN_FILES,true);
            for (String foundTexture : dirFiles){
                auto textureResourceName = path+"/"+foundTexture;
                textureFiles.Push(textureResourceName);
//                Texture* texture = cache->GetResource<Texture>(textureResourceName);
//                if (texture){
//                    textureFiles.Push(textureResourceName);
//                }
            }
        }
    }

    Sort(materialFiles.Begin(),materialFiles.End(),CompareString);
    Sort(techniqueFiles.Begin(),techniqueFiles.End(),CompareString);
   // Sort(textureFiles.Begin(),techniqueFiles.End(),CompareString);


    JSONObject tree;

    tree["id"]=treeID;
    tree["name"]="Tree "+treeID;
    tree["icon"]="COLOR_RED";

    JSONArray nodes;

    {
        // --------------------------------
        // --- PREDEFINED MATERIAL NODE ---
        // --------------------------------
        JSONObject predefMaterialNode;
        NodeSetData(predefMaterialNode,treeID+"__predefMaterialNode","PredefMaterial","Material");

        JSONArray enumElems;
        for (String matName : materialFiles){
            NodeAddEnumElement(enumElems,matName,matName,"Material "+matName,"MATERIAL");
            URHO3D_LOGINFOF("MATERIAL: %s",matName.CString());
        }
        NodeAddPropEnum(predefMaterialNode,"Material",enumElems,"0");

        nodes.Push(predefMaterialNode);
    }

    {
        // --------------------------------
        // ---      MATERIAL NODE       ---
        // --------------------------------
        JSONObject materialNode;

        NodeSetData(materialNode,treeID+"__materialNode","Material","Material");
        NodeAddOutputSocket(materialNode,"defs",SOCK_BOOL);

        nodes.Push(materialNode);
    }

    {
        // --------------------------------
        // ---      PARAMETER NODE      ---
        // --------------------------------
        JSONObject parameterNode;

        NodeSetData(parameterNode,treeID+"__parameterNode","Parameter","Material");
        NodeAddInputSocket(parameterNode,"Material",SOCK_BOOL);

        JSONArray predefNames;
        NodeAddEnumElement(predefNames,"MatDiffColor");
        NodeAddEnumElement(predefNames,"MatSpecColor");
        NodeAddEnumElement(predefNames,"MatEmissiveColor");
        NodeAddEnumElement(predefNames,"MatEnvMapColor");
        NodeAddEnumElement(predefNames,"UOffset");
        NodeAddEnumElement(predefNames,"VOffset");
        NodeAddEnumElement(predefNames,"Roughness");
        NodeAddEnumElement(predefNames,"Metallic");

        NodeAddPropEnum(parameterNode,"name",predefNames,"0");

        NodeAddProp(parameterNode,"value",NT_STRING,"0");

        nodes.Push(parameterNode);
    }

    {
        // --------------------------------
        // ---  CUSTOM PARAMETER NODE   ---
        // --------------------------------
        JSONObject customParameterNode;

        NodeSetData(customParameterNode,treeID+"__customParameterNode","CustomParameter","Material");
        NodeAddInputSocket(customParameterNode,"Material",SOCK_BOOL);
        NodeAddProp(customParameterNode,"key",NT_STRING,"");
        NodeAddProp(customParameterNode,"value",NT_STRING,"");

        nodes.Push(customParameterNode);
    }

    {
        // --------------------------------
        // ---     Standard PARAMS       ---
        // --------------------------------
        JSONObject customParameterNode;

        NodeSetData(customParameterNode,treeID+"__standardParams","StandardParams","Material");
        NodeAddInputSocket(customParameterNode,"Material",SOCK_BOOL);
        NodeAddProp(customParameterNode,"MatDiffColor",NT_COLOR,"(1,1,1,1)");
        NodeAddProp(customParameterNode,"MatSpecColor",NT_COLOR,"(0.1,0.1,0.1,1)");
        NodeAddProp(customParameterNode,"MatEmissiveColor",NT_COLOR,"(0,0,0,1)");
        NodeAddProp(customParameterNode,"UOffset",NT_FLOAT,"0",ST_FACTOR);
        NodeAddProp(customParameterNode,"VOffset",NT_FLOAT,"0",ST_FACTOR);

        nodes.Push(customParameterNode);
    }

    {
        // --------------------------------
        // ---        PBS PARAMS        ---
        // --------------------------------
        JSONObject customParameterNode;

        NodeSetData(customParameterNode,treeID+"__pbsParams","PBSParams","Material");
        NodeAddInputSocket(customParameterNode,"Material",SOCK_BOOL);
        NodeAddProp(customParameterNode,"MatDiffColor",NT_COLOR,"(1,1,1,1)");
        NodeAddProp(customParameterNode,"MatSpecColor",NT_COLOR,"(0.1,0.1,0.1,1)");
        NodeAddProp(customParameterNode,"MatEmissiveColor",NT_COLOR,"(0,0,0,1)");
        NodeAddProp(customParameterNode,"MatEnvMapColor",NT_COLOR,"(1,1,1,1)");
        NodeAddProp(customParameterNode,"Roughness",NT_FLOAT,"0",ST_FACTOR);
        NodeAddProp(customParameterNode,"Metallic",NT_FLOAT,"0",ST_FACTOR);
        NodeAddProp(customParameterNode,"UOffset",NT_FLOAT,"0",ST_FACTOR);
        NodeAddProp(customParameterNode,"VOffset",NT_FLOAT,"0",ST_FACTOR);

        nodes.Push(customParameterNode);
    }



    {
        // --------------------------------
        // ---      Technique NODE       ---
        // --------------------------------
        JSONObject techniqueNode;
        NodeSetData(techniqueNode,treeID+"__techniqueNode","technique","Material");

        // dropdown to choose techniques available from the resource-path
        JSONArray enumElems;
        for (String techniqueName : techniqueFiles){
            NodeAddEnumElement(enumElems,techniqueName,techniqueName,"Technique "+techniqueName,"COLOR");
        }
        NodeAddPropEnum(techniqueNode,"Technique",enumElems,"0");
        // quality
        NodeAddProp(techniqueNode,"quality",NT_INT,"0",ST_NONE,3,0,5);

        NodeAddInputSocket(techniqueNode,"material",SOCK_BOOL);

        nodes.Push(techniqueNode);
    }

    {
        // --------------------------------
        // ---      Texture NODE       ---
        // --------------------------------
        JSONObject textureNode;
        NodeSetData(textureNode,treeID+"__textureNode","texture","Material");


        JSONArray unitElems;
        NodeAddEnumElement(unitElems,"diffuse");
        NodeAddEnumElement(unitElems,"normal");
        NodeAddEnumElement(unitElems,"specular");
        NodeAddEnumElement(unitElems,"emissive");
        NodeAddEnumElement(unitElems,"environment");
        NodeAddEnumElement(unitElems,"0");
        NodeAddEnumElement(unitElems,"1");
        NodeAddEnumElement(unitElems,"2");
        NodeAddEnumElement(unitElems,"3");
        NodeAddEnumElement(unitElems,"4");
        NodeAddEnumElement(unitElems,"5");
        NodeAddPropEnum(textureNode,"unit",unitElems);


        // dropdown to choose textures available from the resource-path
        JSONArray enumElems;
        int counter=0;
        for (String textureName : textureFiles){
            StringHash hash(textureName);
            String id(hash.Value() % 10000000);

            NodeAddEnumElement(enumElems,textureName,textureName,"Texture "+textureName,"COLOR",id);
        }
        NodeAddPropEnum(textureNode,"Texture",enumElems,"0");



        NodeAddInputSocket(textureNode,"material",SOCK_BOOL);

        nodes.Push(textureNode);
    }

    // TODO: create a nodes-builder or similars!?

    tree["nodes"]=nodes;

    return tree;
}

void Urho3DNodeTreeExporter::NodeSetData(JSONObject &node, const String &id, const String &name, const String category)
{
    node["id"]=id;
    node["name"]=name;
    node["category"]=category;
}

void Urho3DNodeTreeExporter::NodeAddProp(JSONObject &node, const String &name, NodeType type, const String& defaultValue, NodeSubType subType, int precission,float min,float max)
{
    JSONObject prop;
    prop["name"]=name;
    prop["default"]=defaultValue;

    switch (type){
        case NT_BOOL: prop["type"] = "bool"; break;
        case NT_INT: prop["type"] = "int"; prop["min"]=(int)min; prop["max"]=(int)max;break;
        case NT_FLOAT: prop["type"] = "float"; prop["min"]=min; prop["max"]=max;prop["precision"]=precission;break;
        case NT_VECTOR2: prop["type"] = "vector2"; break;
        case NT_VECTOR3: prop["type"] = "vector3"; break;
        case NT_VECTOR4: prop["type"] = "vector4"; break;
        case NT_COLOR: prop["type"] = "color"; break;
        case NT_STRING: prop["type"] = "string"; break;
        default:  URHO3D_LOGERRORF("AddProp(%s): Unknown TYPE with int-value:%i",name.CString(),(int)type);
    }
    switch (subType){
        case ST_NONE: prop["subtype"] = "NONE"; break;
        case ST_PIXEL: prop["subtype"] = "PIXEL"; break;
        case ST_ANGLE: prop["subtype"] = "ANGLE"; break;
        case ST_DISTANCE: prop["subtype"] = "DISTANCE"; break;
        case ST_FACTOR: prop["subtype"] = "FACTOR"; break;
        case ST_TIME: prop["subtype"] = "TIME"; break;
        case ST_UNSIGNED: prop["subtype"] = "UNSIGNED"; break;
        default:  URHO3D_LOGERRORF("AddProp(%s): Unknown SUBTYPE with int-value:%i",name.CString(),(int)type);
    }



    if (!node.Contains("props")){
        JSONArray propsArray;
        node["props"]=propsArray;
    }

    // TODO: make this easier
    JSONArray props = node["props"].GetArray();
    props.Push(prop);
    node["props"]=props;
}

void Urho3DNodeTreeExporter::NodeAddPropEnum(JSONObject &node, const String &name, JSONArray &elements, const String &defaultValue)
{
    JSONObject prop;
    prop["name"]=name;
    prop["type"]="enum";
    prop["elements"]=elements;
    prop["default"]=defaultValue;

    if (!node.Contains("props")){
        JSONArray propsArray;
        node["props"]=propsArray;
    }

    // TODO: make this easier
    JSONArray props = node["props"].GetArray();
    props.Push(prop);
    node["props"]=props;
}


void Urho3DNodeTreeExporter::NodeAddEnumElement(JSONArray &elements, const String &id, const String &name, const String &descr, const String &icon, const String& number)
{
    JSONObject elem;
    elem["id"]=id;
    elem["name"]=name==""?id:name;
    elem["description"]=descr==""?id:descr;
    elem["icon"]=icon;
    elem["number"]=number;
    elements.Push(elem);
}

void Urho3DNodeTreeExporter::AddMaterialFolder(const String &folder)
{
    m_materialFolders.Push(folder);
}

void Urho3DNodeTreeExporter::AddTechniqueFolder(const String& folder)
{
    m_techniqueFolders.Push(folder);
}

void Urho3DNodeTreeExporter::AddTextureFolder(const String& folder)
{
    m_textureFolders.Push(folder);
}

void Urho3DNodeTreeExporter::NodeAddSocket(JSONObject &node, const String &name, NodeSocketType type,bool isInputSocket)
{
    String socketlistName = isInputSocket ? "inputsockets" : "outputsockets";

    JSONObject socket;
    socket["name"]=name;
    switch(type){
        case SOCK_BOOL : socket["type"]="bool"; break;
        case SOCK_FLOAT : socket["type"]="float"; break;
        case SOCK_STRING : socket["type"]="string"; break;
        case SOCK_VECTOR : socket["type"]="vector"; break;
    }

    if (!node.Contains(socketlistName)){
        JSONArray sockets;
        node[socketlistName]=sockets;
    }

    // TODO: make this easier
    JSONArray sockets = node[socketlistName].GetArray();
    sockets.Push(socket);
    node[socketlistName]=sockets;
}

void Urho3DNodeTreeExporter::NodeAddInputSocket(JSONObject &node, const String &name, NodeSocketType type)
{
    NodeAddSocket(node,name,type,true);
}

void Urho3DNodeTreeExporter::NodeAddOutputSocket(JSONObject &node, const String &name, NodeSocketType type)
{
    NodeAddSocket(node,name,type,false);
}

JSONObject Urho3DNodeTreeExporter::ExportComponents()
{
    const HashMap<StringHash, SharedPtr<ObjectFactory> >& objFactories = context_->GetObjectFactories();

    auto values = objFactories.Values();

    JSONObject tree;

    String treeID = "urho3dcomponents";

    tree["id"]=treeID;
    tree["name"]="Tree "+treeID;
    tree["icon"]="OUTLINER_OB_GROUP_INSTANCE";

    JSONArray nodes;

    Vector<Pair<String, unsigned> > sortedTypes;
    for (unsigned i = 0; i < values.Size(); ++i)
    {
        SharedPtr<ObjectFactory> val = values.At(i);

        // apply black- /whitelist-Filter
        if (    (InBlacklistMode() && (m_listOfComponents.Contains(val->GetType()) || CheckSuperTypes(val->GetTypeInfo())) )
             || (InWhiteListMode() && (!m_listOfComponents.Contains(val->GetType()) && !CheckSuperTypes(val->GetTypeInfo()) )) )
            continue;

        if (val.NotNull())
        {
            sortedTypes.Push(MakePair(val->GetTypeName(), i));
        }
    }
    Sort(sortedTypes.Begin(), sortedTypes.End());

    const HashMap<StringHash, SharedPtr<ObjectFactory> >& all = context_->GetObjectFactories();

    for (int i=0;i < sortedTypes.Size(); i++){

        auto objectFactoryName = sortedTypes[i].first_;
        JSONObject node;

        SharedPtr<ObjectFactory> val = *all[StringHash(objectFactoryName)];

        if (val->GetTypeInfo()->IsTypeOf(Component::GetTypeInfoStatic())){
       //     URHO3D_LOGINFOF("TYPE:%s\n",val->GetTypeName().CString());
            node["category"]="Component";
        } else {
       //     URHO3D_LOGINFOF("NO COMPONENT: TYPE:%s\n",val->GetTypeName().CString());
        }

        node["id"]=treeID+"__"+val->GetTypeName().Replaced(" ","_");
        node["name"]=val->GetTypeName();

        JSONArray props;

        auto attrs = context_->GetAttributes(val->GetTypeInfo()->GetType());
        if (attrs){
            for (int j = 0;j<attrs->Size();j++){
                auto attr = attrs->At(j);
              //  URHO3D_LOGINFOF("\tattr:%s\n", attr.name_.CString());


                JSONObject prop;
                prop["name"] = attr.name_;
                switch (attr.type_){
                    case VAR_BOOL : prop["type"]="bool"; prop["default"]=attr.defaultValue_.ToString(); break;
                    case VAR_INT : {
                        if (!attr.enumNames_) {
                            prop["type"]="int"; prop["default"]=attr.defaultValue_.ToString();
                        } else {
                            prop["type"]="enum"; prop["default"]=attr.defaultValue_.ToString();

                            JSONArray elements;
                            for (int idx = 0; attr.enumNames_[idx] != NULL; idx++)
                            {
                                JSONObject elem;
                                elem["id"]=attr.enumNames_[idx];
                                elem["name"]=attr.enumNames_[idx];
                                elements.Push(elem);
                            }

                            prop["elements"]=elements;
                        }
                        break;
                    }
                    case VAR_FLOAT : prop["type"]="float"; prop["default"]=attr.defaultValue_.ToString();break;
                    case VAR_STRING : prop["type"]="string"; prop["default"]=attr.defaultValue_.ToString();break;
                    case VAR_COLOR : prop["type"]="color"; prop["default"]="("+attr.defaultValue_.ToString().Replaced(" ",",")+")";break;
                    case VAR_VECTOR2 : prop["type"]="vector2";prop["default"]="("+attr.defaultValue_.ToString().Replaced(" ",",")+")"; break;
                    case VAR_VECTOR3 : prop["type"]="vector3";prop["default"]="("+attr.defaultValue_.ToString().Replaced(" ",",")+")"; break;
                    case VAR_VECTOR4 : prop["type"]="vector4";prop["default"]="("+attr.defaultValue_.ToString().Replaced(" ",",")+")"; break;
                    default:
                        URHO3D_LOGINFOF("[%s] Skipping attribute:%s",val->GetTypeName().CString(),attr.name_.CString());
                        continue;

                }

                props.Push(prop);
            }
        }
        node["props"]=props;
        nodes.Push(node);

    }
    tree["nodes"]=nodes;
    return tree;
}

void Urho3DNodeTreeExporter::Export(String filename)
{
    auto componentTree = ExportComponents();
    auto materialTree = ExportMaterials();

    trees.Push(componentTree);
    trees.Push(materialTree);

    if (!m_customUIFilenames.Empty()){
        FileSystem* fs = GetSubsystem<FileSystem>();
        JSONArray jsonCustomUIs;
        for (auto m_customUIFilename : m_customUIFilenames)
        {
            if (!fs->FileExists(m_customUIFilename)){
                URHO3D_LOGERRORF("File %s not found",m_customUIFilename.CString());
            } else {
                File file(context_);
                file.Open(m_customUIFilename);
                String allText="";
                while (!file.IsEof()){
                    String line = file.ReadLine()+"\n";
                    allText += line;
                }
                URHO3D_LOGINFO("READ FILE:");
                URHO3D_LOGINFO(allText.CString());

                auto e = base64_encode(reinterpret_cast<const unsigned char*>(allText.CString()),allText.Length());
                String enc(e.c_str());
                String dec(base64_decode(enc.CString()).c_str());


                jsonCustomUIs.Push(enc);
            }
        }
        fileRoot["customUI"] = jsonCustomUIs;
    }

    fileRoot["trees"]=trees;

    JSONFile file(context_);
    file.GetRoot() = fileRoot;
    file.SaveFile(filename);

}

void Urho3DNodeTreeExporter::AddCustomUIFile(const String &filename)
{
     m_customUIFilenames.Push(filename);
}
