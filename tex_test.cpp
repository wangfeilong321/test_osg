#include "stdafx.h"
#include "find_node_visitor.h" 
#include "creators.h"
#include "info_visitor.h"

osg::Image* createSpotLight( const osg::Vec4& centerColor, const osg::Vec4& bgColor, unsigned int size, float power )
{
    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->allocateImage( size, size, 1, GL_RGBA, GL_UNSIGNED_BYTE );

    float mid = (float(size)-1) * 0.5f;
    float div = 2.0f / float(size);
    for( unsigned int r=0; r<size; ++r )
    {
        unsigned char* ptr = image->data(0, r, 0);
        for( unsigned int c=0; c<size; ++c )
        {
            float dx = (float(c) - mid)*div;
            float dy = (float(r) - mid)*div;
            float r = powf(1.0f - sqrtf(dx*dx+dy*dy), power);
            if ( r<0.0f ) r = 0.0f;

            osg::Vec4 color = centerColor*r + bgColor*(1.0f - r);
            *ptr++ = (unsigned char)((color[0]) * 255.0f);
            *ptr++ = (unsigned char)((color[1]) * 255.0f);
            *ptr++ = (unsigned char)((color[2]) * 255.0f);
            *ptr++ = (unsigned char)((color[3]) * 255.0f);
        }
    }
    return image.release();
}

class FindTextureVisitor : public osg::NodeVisitor
{
public:
    FindTextureVisitor( osg::Texture* tex ) : _texture(tex)
    {
        setTraversalMode( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN );
    }

    virtual void apply( osg::Node& node )
    {
        replaceTexture( node.getStateSet() );
        traverse( node );
    }

    virtual void apply( osg::Geode& geode )
    {
        replaceTexture( geode.getStateSet() );
        for ( unsigned int i=0; i<geode.getNumDrawables(); ++i )
        {
            replaceTexture( geode.getDrawable(i)->getStateSet() );
        }
        traverse( geode );
    }

    void replaceTexture( osg::StateSet* stateset )
    {
        if ( stateset )
        {

            size_t tl_s = stateset->getTextureAttributeList().size();
            
            for(unsigned i = 0;i< tl_s;i++)
            {
                osg::Texture* oldTexture = dynamic_cast<osg::Texture*>(
                    stateset->getTextureAttribute(i, osg::StateAttribute::TEXTURE) );

                if ( oldTexture )
                {  
                    if(!_old_texture) _old_texture = new  osg::Texture2D(oldTexture->getImage(0));
                   stateset->setTextureAttribute( i, _texture.get() );
                   
                }
            }
        }
    }

    inline osg::Texture* getOldTexture()
    {
         return  _old_texture.release();
    }

protected:
    osg::ref_ptr<osg::Texture> _texture;
    osg::ref_ptr<osg::Texture> _old_texture;
};

class TexChangeHandler : public osgGA::GUIEventHandler
{
public:
    TexChangeHandler(osg::Node * root,osg::Texture *texture) 
      :_texture    (texture)
      /*,_old_texture(texture)*/
      ,_root       (root)
      {}

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        if (!ea.getHandled() && ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN)
        {            
            if (ea.getKey()=='<' || ea.getKey()=='>')
            {
               FindTextureVisitor ftv( _texture.get() );
                if ( _root )
                { 
                    _root->accept( ftv );
                    _texture =  ftv.getOldTexture();
                }
                return true;
            }
        }

        return false;
    }

    virtual void getUsage(osg::ApplicationUsage& usage) const
    {
        usage.addKeyboardMouseBinding("Keypad +",       "Increment time by one minute");
        usage.addKeyboardMouseBinding("Shift Keypad +", "Increment time by one hour"  );
        usage.addKeyboardMouseBinding("Alt Keypad +",   "Increment time by one day"   );
        usage.addKeyboardMouseBinding("Ctrl Keypad +",  "Increment time by one month" );
        usage.addKeyboardMouseBinding("Keypad -",       "Decrement time by one minute");
        usage.addKeyboardMouseBinding("Shift Keypad -", "Decrement time by one hour"  );
        usage.addKeyboardMouseBinding("Alt Keypad -",   "Decrement time by one day"   );
        usage.addKeyboardMouseBinding("Ctrl Keypad -",  "Decrement time by one month" );
    }

    osg::ref_ptr<osg::Texture> _texture;
    //osg::ref_ptr<osg::Texture> _old_texture;
    osg::ref_ptr<osg::Node> _root;
};

int main_tex_test( int argc, char** argv )
{

    osgViewer::Viewer viewer;

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    //root->setMatrix(osg::Matrix::rotate(osg::inDegrees(-90.0f),1.0f,0.0f,0.0f));
#if 0
    //int tex_width = 2048, tex_height = 2048;
    //osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
    //texture->setTextureSize( tex_width, tex_height );
    //texture->setInternalFormat( GL_RGBA );
    //texture->setFilter( osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR );
    //texture->setFilter( osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR );
#endif
#if 0
    osg::Vec4 centerColor( 1.0f, 1.0f, 0.0f, 1.0f );
    osg::Vec4 bgColor( 0.0f, 0.0f, 0.0f, 1.0f );

    osg::ref_ptr<osg::ImageSequence> imageSequence = new osg::ImageSequence;
    imageSequence->addImage( createSpotLight(centerColor, bgColor, 2048, 3.0f) );
    imageSequence->addImage( createSpotLight(centerColor, bgColor, 2048, 3.5f) );
    imageSequence->addImage( createSpotLight(centerColor, bgColor, 2048, 4.0f) );
    imageSequence->addImage( createSpotLight(centerColor, bgColor, 2048, 3.5f) );

    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
    texture->setImage( imageSequence.get() );
#endif

    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D(osgDB::readImageFile("a_319_aeroflot.png"/*"Fieldstone.jpg"*/));

    creators::nodes_array_t plane = creators::loadAirplaneParts();
    auto airplane = plane[1];

    // osg::Node*  airplane = // creators::loadAirplane();
    
    InfoVisitor infoVisitor;
    airplane->accept( infoVisitor );

    findNodeVisitor findNode("Body_",findNodeVisitor::not_exact); 
    airplane->accept(findNode);
    auto a_node =  findNode.getFirst();
    
    findNodeByType<osg::Geode> findGeode; 
    a_node->accept(findGeode);
    auto g_node =  findGeode.getFirst();
    
    //auto dr = g_node->asGeode()->getDrawable(0);
    //
    //auto dr_num =  g_node->asGeode()->getNumDrawables();

    // g_node->setNodeMask(0);

    //osg::StateSet* stateset = g_node->asGeode()->getDrawable(0)->getOrCreateStateSet();
    // � ��� ��� � ��� ���������� �� ����� ����
    // osg::Array* a = g_node->asGeode()->getDrawable(0)->asGeometry()->getTexCoordArray(1);

    //stateset->setTextureAttributeAndModes( 0, /*texture.get()*/new osg::Texture2D());
    //g_node->setStateSet(stateset);
    
    osg::ref_ptr<osg::Light> light = new osg::Light;
    light->setLightNum( 0 );
    light->setAmbient(osg::Vec4(0.2, 0.2, 0.2, 1));
    light->setDiffuse(osg::Vec4(0.8, 0.8, 0.8, 1));
    light->setPosition( osg::Vec4(100.0f, 10.0f, 10.0f, 1.0f) );

    osg::ref_ptr<osg::LightSource> source = new osg::LightSource;
    source->setLight( light ); 
    
    osg::ref_ptr<osgFX::BumpMapping> effet = new osgFX::BumpMapping();
    effet->setLightNumber(0);
    effet->setOverrideDiffuseTexture(texture.get());
    effet->setOverrideNormalMapTexture(new osg::Texture2D(osgDB::readImageFile("a_319_n.png")));
    effet->addChild(airplane);
    // effet->prepareChildren();
    effet->setEnabled(false);


    // effet->setUpDemo();
    // effet->setEnabled(false);
    root->addChild(source);
    root->addChild(effet);

    //FindTextureVisitor ftv( texture.get() );
    //if ( root ) root->accept( ftv );
    
    // osgDB::writeNodeFile(*root,"tex_test_blank.osgt");

    viewer.addEventHandler( new TexChangeHandler( root.get(), texture.get() ) );
    // Add some useful handlers to see stats, wireframe and onscreen help
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(root->getOrCreateStateSet()));
    viewer.addEventHandler(new osgViewer::HelpHandler);
    viewer.setSceneData( root.get() );
    return viewer.run();
}