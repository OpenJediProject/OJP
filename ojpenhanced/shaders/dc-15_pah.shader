gfx/effects/dc-15frontflash
{
    {
           map gfx/effects/dc-15frontflash
           blendFunc add
    }
}

gfx/effects/dc-15sideflash
{
    {
           map gfx/effects/dc-15sideflash
           blendFunc add
    }
}

models/weapons2/heavy_repeater/barrel
{
	q3map_nolightmap
    {
        map models/weapons2/heavy_repeater/barrel
        rgbGen lightingDiffuse
    }
    {
        map models/weapons2/heavy_repeater/barrel_spec
        blendFunc GL_SRC_ALPHA GL_ONE
        detail
        alphaGen lightingSpecular
    }
}

models/weapons2/heavy_repeater/case
{
	q3map_nolightmap
    {
        map models/weapons2/heavy_repeater/case
        rgbGen lightingDiffuse
    }
    {
        map models/weapons2/heavy_repeater/case_spec
        blendFunc GL_SRC_ALPHA GL_ONE
        detail
        alphaGen lightingSpecular
    }
}

models/weapons2/heavy_repeater/shaft
{
	q3map_nolightmap
    {
        map models/weapons2/heavy_repeater/shaft
        rgbGen lightingDiffuse
    }
    {
        map models/weapons2/heavy_repeater/shaft_spec
        blendFunc GL_SRC_ALPHA GL_ONE
        detail
        alphaGen lightingSpecular
    }
}