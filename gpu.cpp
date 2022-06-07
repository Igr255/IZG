/*!
 * @file
 * @brief This file contains implementation of gpu
 *
 * @author Tomáš Milet, imilet@fit.vutbr.cz
 */

#include <student/gpu.hpp>
#include <cstdio>

struct Triangle{
    OutVertex points[3];
};

float MAX(float x, float x1);

float MIN(float y, float y1);

void vertexAssembly(VertexAttrib buf_ptr, u_int32_t j, InVertex *inVertex){
    if(buf_ptr.type == AttributeType::FLOAT)
        inVertex->attributes[j].v1 = *(float *) ( (uint8_t *)buf_ptr.bufferData + buf_ptr.offset + buf_ptr.stride * inVertex->gl_VertexID );
    if(buf_ptr.type == AttributeType::VEC2)
        inVertex->attributes[j].v2 = *(glm::vec2 *) ( (uint8_t *)buf_ptr.bufferData + buf_ptr.offset + buf_ptr.stride * inVertex->gl_VertexID );
    if(buf_ptr.type == AttributeType::VEC3)
        inVertex->attributes[j].v3 = *(glm::vec3 *) ( (uint8_t *)buf_ptr.bufferData + buf_ptr.offset + buf_ptr.stride * inVertex->gl_VertexID );
    if(buf_ptr.type == AttributeType::VEC4)
        inVertex->attributes[j].v4 = *(glm::vec4 *) ( (uint8_t *)buf_ptr.bufferData + buf_ptr.offset + buf_ptr.stride * inVertex->gl_VertexID );
}

uint32_t  computeVertexID(VertexArray const&vao, uint32_t shaderInvocation){

    if (vao.indexType == IndexType::UINT16){
        uint16_t *ind = (uint16_t*)vao.indexBuffer;
        return  ind[shaderInvocation];
    }

    else if (vao.indexType == IndexType::UINT8){
        uint8_t *ind = (uint8_t *) vao.indexBuffer;
        return ind[shaderInvocation];
    }

    else{
        uint32_t *ind = (uint32_t *) vao.indexBuffer;
        return ind[shaderInvocation];
    }
}


void rasterize(Triangle triangle, Frame frame, Program prg){

    glm::vec4 v1 = triangle.points[0].gl_Position;
    glm::vec4 v2 = triangle.points[1].gl_Position;
    glm::vec4 v3 = triangle.points[2].gl_Position;

    InFragment inFrag;
    OutFragment outFragment;



    int maxX = MAX(v1.x, MAX(v2.x, v3.x));
    int minX = MIN(v1.x, MIN(v2.x, v3.x));

    int maxY = MAX(v1.y, MAX(v2.y, v3.y));
    int minY = MIN(v1.y, MIN(v2.y, v3.y));

    minX = MAX(0, minX);
    minY = MAX(0, minY);

    maxX = MIN(frame.width - 1, maxX);
    maxY = MIN(frame.height - 1, maxY);


    int deltaX1 = v2.x - v1.x;
    int deltaX2 = v3.x - v2.x;
    int deltaX3 = v1.x - v3.x;

    int deltaY1 = v2.y - v1.y;
    int deltaY2 = v3.y - v2.y;
    int deltaY3 = v1.y - v3.y;

    int edgeF1 = (minY - v1.y) * deltaX1 - (minX - v1.x) * deltaY1;
    int edgeF2 = (minY - v2.y) * deltaX2 - (minX - v2.x) * deltaY2;
    int edgeF3 = (minY - v3.y) * deltaX3 - (minX - v3.x) * deltaY3;


    for (int y = minY; y <= maxY; y++) {

        bool even = (y - minY) % 2 == 0;

        int startX = even ? minX : maxX;
        int endX = even ? maxX + 1 : minX - 1;
        int stepX = even ? 1 : -1;

        for (int x = startX; x != endX; x += stepX)
        {
            if (edgeF1 >= 0 && edgeF2 >= 0 && edgeF3 >= 0)
            {
                prg.fragmentShader(outFragment,inFrag,prg.uniforms);
            }
            if (!((even && x == maxX) || (!even && x == minX)))
            {
                edgeF1 += even ? -deltaY1 : deltaY1;
                edgeF2 += even ? -deltaY2 : deltaY2;
                edgeF3 += even ? -deltaY3 : deltaY3;
            }
        }

        edgeF1 += deltaX1;
        edgeF2 += deltaX2;
        edgeF3 += deltaX3;
    }

}

float MIN(float y, float y1) {
    if (y < y1)
        return y;
    return y1;
}

float MAX(float x, float x1) {
    if (x > x1)
        return x;
    return x1;
}


glm::vec4 perspectiveDivision(glm::vec4 point) {
    point[0] = point[0] / point[3];
    point[1] = point[1] / point[3];

    return point;
}

glm::vec4 viewpointTransformation(glm::vec4 point, Frame *frame) {
    auto width = frame->width;
    auto height = frame->height;

    point[0] = point[0] * (width / 2) + (width / 2);
    point[1] = point[1] * (height / 2) + (height / 2);

    return point;
}


//! [drawTrianglesImpl]
void drawTrianglesImpl(GPUContext &ctx,uint32_t nofVertices){
    (void)ctx;
    (void)nofVertices;
    /// \todo Tato funkce vykreslí trojúhelníky podle daného nastavení.<br>
    /// ctx obsahuje aktuální stav grafické karty.
    /// Parametr "nofVertices" obsahuje počet vrcholů, který by se měl vykreslit (3 pro jeden trojúhelník).<br>
    /// Bližší informace jsou uvedeny na hlavní stránce dokumentace.

    //Triangle triangle;
    InVertex inVertex;
    OutVertex outVertex;

    glm::vec4 pointVector;
    Triangle triangle;
    int counter = 0;

    for (uint32_t i = 0; i < nofVertices; ++i) {
        //printf("-- %i --\n", nofVertices);


        if (ctx.vao.indexBuffer != nullptr)
            inVertex.gl_VertexID = computeVertexID(ctx.vao, i);
        else
            inVertex.gl_VertexID = i;
        int e = 0;

        for(uint32_t j=0;j<maxAttributes;++j){
            VertexAttrib buf_ptr = ctx.vao.vertexAttrib[j];
            if(!buf_ptr.bufferData)
                continue;

            vertexAssembly(buf_ptr, j, &inVertex);

            e++;

            //printf("====%i====\n", e);


        }

        ctx.prg.vertexShader(outVertex, inVertex, ctx.prg.uniforms);

        /// loading triangle points and then whole triangle ///

        pointVector = outVertex.gl_Position;

        pointVector = perspectiveDivision(pointVector);
        pointVector = viewpointTransformation(pointVector, &ctx.frame);

        if (counter < 3) {
            triangle.points[counter].gl_Position = pointVector;
            //printf("0 %f %f %f \n", triangle.points[0].gl_Position.x, triangle.points[0].gl_Position.y, triangle.points[0].gl_Position.z);
            //printf("1 %f %f %f \n", triangle.points[1].gl_Position.x, triangle.points[1].gl_Position.y, triangle.points[1].gl_Position.z);
            //printf("2 %f %f %f \n", triangle.points[2].gl_Position.x, triangle.points[2].gl_Position.y, triangle.points[2].gl_Position.z);

        }
        counter++;

        // triangle is loaded can be rasterised but idk how yeehaw
        if (counter == 3) {
            //printf("==== dO IT ====\n");
            rasterize(triangle, ctx.frame, ctx.prg);

        }


        ///////////////////////////////////////////////////////
    }

}


//! [drawTrianglesImpl]

/**
 * @brief This function reads color from texture.
 *
 * @param texture texture
 * @param uv uv coordinates
 *
 * @return color 4 floats
 */
glm::vec4 read_texture(Texture const&texture,glm::vec2 uv){
    if(!texture.data)return glm::vec4(0.f);
    auto uv1 = glm::fract(uv);
    auto uv2 = uv1*glm::vec2(texture.width-1,texture.height-1)+0.5f;
    auto pix = glm::uvec2(uv2);
    //auto t   = glm::fract(uv2);
    glm::vec4 color = glm::vec4(0.f,0.f,0.f,1.f);
    for(uint32_t c=0;c<texture.channels;++c)
        color[c] = texture.data[(pix.y*texture.width+pix.x)*texture.channels+c]/255.f;
    return color;
}

/**
 * @brief This function clears framebuffer.
 *
 * @param ctx GPUContext
 * @param r red channel
 * @param g green channel
 * @param b blue channel
 * @param a alpha channel
 */
void clear(GPUContext&ctx,float r,float g,float b,float a){
    auto&frame = ctx.frame;
    auto const nofPixels = frame.width * frame.height;
    for(size_t i=0;i<nofPixels;++i){
        frame.depth[i] = 10e10f;
        frame.color[i*4+0] = static_cast<uint8_t>(glm::min(r*255.f,255.f));
        frame.color[i*4+1] = static_cast<uint8_t>(glm::min(g*255.f,255.f));
        frame.color[i*4+2] = static_cast<uint8_t>(glm::min(b*255.f,255.f));
        frame.color[i*4+3] = static_cast<uint8_t>(glm::min(a*255.f,255.f));
    }
}

