#ifndef N_FLOATANIMATOR_H
#define N_FLOATANIMATOR_H
//------------------------------------------------------------------------------
/**
    @class nFloatAnimator
    @ingroup SceneAnimators

    @brief Animates a float attribute of a nAbstractShaderNode.

    (C) 2004 RadonLabs GmbH
*/
#include "scene/nanimator.h"
#include "gfx2/nshader2.h"

//------------------------------------------------------------------------------
class nFloatAnimator : public nAnimator
{
public:
    /// constructor
    nFloatAnimator();
    /// destructor
    virtual ~nFloatAnimator();
    /// save object to persistent stream
    virtual bool SaveCmds(nPersistServer* ps);

    /// return the type of this animator object (SHADER)
    virtual Type GetAnimatorType() const;
    /// called by scene node objects which wish to be animated by this object
    virtual void Animate(nSceneNode* sceneNode, nRenderContext* renderContext);
    /// set the name of the float parameter to manipulate
    void SetVectorName(const char* name);
    /// get the name of the float parameter to manipulate
    const char* GetVectorName();
    /// add a key 
    void AddKey(float time, const float& key);
    /// get number of keys
    int GetNumKeys() const;
    /// get key at
    void GetKeyAt(int index, float& time, float& key) const;

protected:
    /// a key class with a time and a int key value
    class Key
    {
    public:
        /// default constructor
        Key();
        /// constructor
        Key(float t, const float& v);

        float time;
        float value;
    };
    /// get an interpolated key
    bool SampleKey(float time, const nArray<Key>& keyArray, float& result);

private:
    nShaderState::Param vectorParameter;
    nArray<Key> keyArray;
};

//------------------------------------------------------------------------------
/**
*/
inline
nFloatAnimator::Key::Key()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nFloatAnimator::Key::Key(float t, const float& i) :
    time(t),
    value(i)
{
    // empty
}

//------------------------------------------------------------------------------
#endif