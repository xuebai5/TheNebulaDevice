#ifndef N_CLASS_H
#define N_CLASS_H
//------------------------------------------------------------------------------
/**
    @class nClass

    Nebula metaclass. nRoot derived objects are not created directly
    in C++, but by nClass objects. nClass objects wrap dynamic demand-loading
    of classes, and do other householding stuff.

    (C) 2002 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include "kernel/ncmdproto.h"
#include "util/nkeyarray.h"

//------------------------------------------------------------------------------
class nCmdProtoNative;
class nRoot;
class nKernelServer;
class nHashList;
class nClass : public nHashNode
{
public:
    /// constructor
    nClass(const char* name, nKernelServer *ks, bool (*)(nClass *, nKernelServer *), void (*)(void), void *(*)(void));
    /// class destructor
    ~nClass();
    /// create a new instance of the class
    nRoot* NewObject();
    /// start defining commands
    void BeginCmds();
    /// add a command to the class
    void AddCmd(const char *proto_def, uint id, void (*)(void *, nCmd *));
    /// finish defining commands
    void EndCmds();
    /// start defining script-side commands
    void BeginScriptCmds(int numCmds);
    /// add a script-side command for this class
    void AddScriptCmd(nCmdProto*);
    /// finish defining script-side commands
    void EndScriptCmds();
    /// find command by name (searches both native & script-side)
    nCmdProto* FindCmdByName(const char *name);
    /// find a native command by name
    nCmdProtoNative *FindNativeCmdByName(const char *name);
    /// find a script-side command by name
    nCmdProto *FindScriptCmdByName(const char *name);
    /// find a native command by fourcc code
    nCmdProto* FindCmdById(uint id);
    /// get pointer to command list
    nHashList* GetCmdList() const;
    /// get super class of this class
    nClass *GetSuperClass() const;
    /// add a sub class to this class
    void AddSubClass(nClass* cl);
    /// remove a subclass from this class
    void RemSubClass(nClass* cl);
    /// increment ref count of class object
    int AddRef();
    /// decrement ref count of class object
    int RemRef();
    /// get current refcount of class object
    int GetRef() const;
    /// set instance size
    void SetInstanceSize(int size);
    /// get instance size
    int GetInstanceSize() const;

private:
    nKernelServer* kernelServer;
    nClass* superClass;
    nHashList* cmdList;
    nKeyArray<nCmdProto*>* cmdTable;
    /// The hashed script commandlist of this class
    nHashList* script_cmd_list;
    int refCount;
    int instanceSize;

    bool (*n_init_ptr)(nClass *, nKernelServer *);      // pointer to class init function
    void (*n_fini_ptr)(void);                           // pointer to class fini function
    void *(*n_new_ptr)(void);                           // pointer to object construction function
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nClass::SetInstanceSize(int size)
{
    this->instanceSize = size;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nClass::GetInstanceSize() const
{
    return this->instanceSize;
}

//------------------------------------------------------------------------------
/**
*/
inline
nClass*
nClass::GetSuperClass() const
{
    return this->superClass;
}

//------------------------------------------------------------------------------
/**
*/
inline
void 
nClass::AddSubClass(nClass *cl)
{
    this->AddRef();
    cl->superClass = this;
}

//------------------------------------------------------------------------------
/**
*/
inline
void 
nClass::RemSubClass(nClass *cl)
{
    this->RemRef();
    cl->superClass = NULL;
}        

//------------------------------------------------------------------------------
/**
*/
inline
int
nClass::AddRef()
{
    return ++this->refCount;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nClass::RemRef()
{
    n_assert(this->refCount > 0);
    return --this->refCount;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nClass::GetRef() const
{
    return this->refCount;
}

//------------------------------------------------------------------------------
/**
*/
inline
nHashList* 
nClass::GetCmdList() const
{
    return this->cmdList;
}

//------------------------------------------------------------------------------
#endif
