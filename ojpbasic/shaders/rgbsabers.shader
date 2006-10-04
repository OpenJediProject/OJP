//[RGBSabers]
gfx/effects/sabers/rgb_glow
{
	cull	twosided
    {
        map gfx/effects/sabers/rgb_glow2
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/effects/sabers/rgb_line
{
	cull	twosided
    {
        clampmap gfx/effects/sabers/rgb_line
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/sabers/rgb_core
{
	cull	twosided
    {
        clampmap gfx/effects/sabers/rgb_core
        blendFunc GL_ONE GL_ONE
        rgbGen identity
    }
}

gfx/effects/sabers/black_glow
{
	cull	twosided
    {
        clampmap gfx/effects/sabers/black_glow
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        glow
        rgbGen identity
        alphaGen vertex
    }
}

gfx/effects/sabers/black_line
{
	cull	twosided
    {
        clampmap gfx/effects/sabers/black_line
        blendFunc GL_ONE GL_ONE
        rgbGen identity
    }
}

gfx/effects/sabers/blacksaberBlur
{
	cull	twosided
    {
        clampmap gfx/effects/sabers/black_trail_glow
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        glow
        rgbGen identity
        alphaGen vertex
    }
    {
        clampmap gfx/effects/sabers/blurcore
        blendFunc GL_ONE GL_ONE
        rgbGen identity
    }
}
