// This file is a central repository
// of objects we can dereference and
// draw with our drawing functions.
// The drawing functions just loop through
// our arrays here and draw what's in them
// according to a custom protocol for
// each type of object.

// Geometry (map and players)
const _geoElements = [];

export function getGeoElements()
{
    return _geoElements;
}

// HUD objects
const _hudElements = [];

export function getHudElements()
{
    return _hudElements;
}

// Text objects
const _textElements = [];

export function getTextElements()
{
    return _textElements;
}

export function addTextElement(element)
{
    _textElements.push(element);
}

export function removeTextElement()
{

}
