//------------------------------------------------------------------------------
//  nshapenode_main.cc
//  (C) 2002 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "scene/nshapenode.h"
#include "gfx2/nmesh2.h"
#include "gfx2/ngfxserver2.h"

nNebulaScriptClass(nShapeNode, "nmaterialnode");

//------------------------------------------------------------------------------
/**
*/
nShapeNode::nShapeNode() :
    groupIndex(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nShapeNode::~nShapeNode()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    This method must return the mesh usage flag combination required by
    this shape node class. Subclasses should override this method
    based on their requirements.

    @return     a combination on nMesh2::Usage flags
*/
int
nShapeNode::GetMeshUsage() const
{
    return nMesh2::WriteOnce;
}

//------------------------------------------------------------------------------
/**
    Unload mesh resource if valid.
*/
void
nShapeNode::UnloadMesh()
{
    if (this->refMesh.isvalid())
    {
        this->refMesh->Release();
        this->refMesh.invalidate();
    }
}

//------------------------------------------------------------------------------
/**
    Load new mesh, release old one if valid. Also initializes the groupIndex
    member.
*/
bool
nShapeNode::LoadMesh()
{
    if ((!this->refMesh.isvalid()) && (!this->meshName.IsEmpty()))
    {
        //get a mesh, maybe reuse an existing one.
        nMesh2* mesh = this->refGfxServer->NewMesh(this->meshName.Get());
        n_assert(mesh);        
        
        if (mesh->IsValid())
        {
            //if we got a mesh, validate that we can reuse it
            if (mesh->GetUsage() != this->GetMeshUsage())
            {
                nString resourceName = this->meshName.Get();
                resourceName += "_"; resourceName += this->GetMeshUsage();
                //if the usage is not equal create a usage type specific one
                mesh = this->refGfxServer->NewMesh(resourceName.Get());
                n_assert(mesh);
            }
        }
        
        if (!mesh->IsValid())
        {
            mesh->SetFilename(this->meshName.Get());
            mesh->SetUsage(this->GetMeshUsage());

            if (refMeshResourceLoader.isvalid())
            {
                mesh->SetResourceLoader(refMeshResourceLoader.getname());
            }

            if (!mesh->Load())
            {
                n_printf("nMeshNode: Error loading mesh '%s'\n", this->meshName.Get());
                mesh->Release();
                return false;
            }
        }
        this->refMesh = mesh;
        this->SetLocalBox(this->refMesh->GetGroup(this->groupIndex).GetBoundingBox());
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Load the resources needed by this object.
*/
bool
nShapeNode::LoadResources()
{
    if (nMaterialNode::LoadResources())
    {
        this->LoadMesh();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Unload the resources if refcount has reached zero.
*/
void
nShapeNode::UnloadResources()
{
    nMaterialNode::UnloadResources();
    this->UnloadMesh();
}

//------------------------------------------------------------------------------
/**
    Indicate to scene server that we provide geometry
*/
bool
nShapeNode::HasGeometry() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
    Update geometry, set as current mesh in the gfx server and
    call nGfxServer2::DrawIndexed().
*/
bool
nShapeNode::RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext)
{
    n_assert(sceneServer);
    n_assert(renderContext);
    nGfxServer2* gfx = this->refGfxServer.get();

    // TODO call geometry manipulators!

    // see if resources need to be reloaded
    if (!this->AreResourcesValid())
    {
        this->LoadResources();
    }
    n_assert(this->refMesh->IsValid());

    // render the mesh in normal mode (always at stream 0)
    gfx->SetMesh(0, this->refMesh.get());

    // set the vertex and index range
    const nMeshGroup& curGroup = this->refMesh->GetGroup(this->groupIndex);
    gfx->SetVertexRange(curGroup.GetFirstVertex(), curGroup.GetNumVertices());
    gfx->SetIndexRange(curGroup.GetFirstIndex(), curGroup.GetNumIndices());
    gfx->DrawIndexed(TriangleList);
    return true;
}

//------------------------------------------------------------------------------
/**
    Set the resource name. The mesh resource name consists of the
    filename of the mesh, and optionally a mesh group name, delimited by '|'.
    If no group name exists, the first group in the mesh file will be used.
*/
void
nShapeNode::SetMesh(const char* name)
{
    n_assert(name);
    this->UnloadMesh();
    this->meshName = name;
}

//------------------------------------------------------------------------------
/**
*/
const char*
nShapeNode::GetMesh() const
{
    return this->meshName.IsEmpty() ? 0 : this->meshName.Get();
}

//------------------------------------------------------------------------------
/**
    Set the resource loader used to load the mesh data.  If this is NULL, then
    the mesh is loaded through the default mesh loading code.
*/
void
nShapeNode::SetMeshResourceLoader(const char* resourceLoaderPath)
{
    this->refMeshResourceLoader = resourceLoaderPath;
}

//------------------------------------------------------------------------------
/**
    Get the mesh resource loader.
*/
const char *
nShapeNode::GetMeshResourceLoader()
{
    return this->refMeshResourceLoader.getname();
}

