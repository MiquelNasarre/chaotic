#pragma once
#include "Drawable.h"

/* LIGHT DRAWABLE CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
Drawable class to draw point light sources in the scene. Given this library supports 
illumination it is only natural to have a drawable to represent such light sources.

This object allows for drawing of light sources anywhere in you scene with any color or
intensity. Note that the are not actual light sources for other drawables and you will 
have to call their own updateLight() function to add any additional light.

Lights should be drawn after all other drawables have been drawn and thay are single 
point fully transparent light sources, so they only add light to the pixels.

Unfortunately lights have poor interaction with transparent objects. Due to the nature
of OIT the light glow state can not be reconciled, so lights will always appear behind 
transparent objects even if they are acutally in front.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Light descriptor struct, to be created and passed as a pointer to initialize
// a Light object. Sets the ligth color, position, intensity and misc.
struct LIGHT_DESC
{
	// Defines the color of the light source. Alpha value is 
	// ignored because lighting is always additive.
	Color color = Color::White;

	// Defines the R3 position of the light source.
	Vector3f position = {};
	
	// Defines the intensity of the light source.
	float intensity = 1.f;
	
	// Defines the radius in 3D coordinates of the circle the
	// drawable will call the pixel shader on to add the light.
	float radius = 1.f;

	// Defines the number of sides the poilhedron used to create 
	// the circle will have. Minimum value is three.
	unsigned poligon_sides = 40u;
};

// Light drawable class, used for drawing and interacting with single point transparent light 
// sources on a Graphics instance. Allows for color, position and intensity updates.
class Light : public Drawable
{
public:
	// Light constructor, if the pointer is valid it will call the initializer.
	Light(const LIGHT_DESC* pDesc = nullptr);

	// Frees the GPU pointers and all the stored data.
	~Light();

	// Initializes the Light object, it expects a valid pointer to a descriptor
	// and will initialize everything as specified, can only be called once per object.
	void initialize(const LIGHT_DESC* pDesc);

	// Updates the intensity value of the light.
	void updateIntensity(float intensity);

	// Updates the scene position of the Light. If additive it will add the vector
	// to the current position vector of the Light.
	void updatePosition(Vector3f position, bool additive = false);

	// Updates the color of the Light.
	void updateColor(Color color);

	// Updates the maximum radius of the Light.
	void updateRadius(float radius);

	// Returns the current light intensity.
	float getIntensity() const;

	// Returns the current light color.
	Color getColor() const;

	// Returns the current scene position.
	Vector3f getPosition() const;

	// Returns the current maximum radius.
	float getRadius() const;

private:
	// Pointer to the internal class storage.
	void* lightData = nullptr;
};
