export const name = "RGB Cycle"
export const interval = 33 // ms

let degrees = 0

export function tick(baseColor, colors)
{
	baseColor = new QColor(baseColor)
	baseColor.setHsl(baseColor.hslHue() + degrees++, baseColor.hslSaturation(), baseColor.lightness())

	if (degrees > 360)
		degrees = 0

	return colors.fill(baseColor.rgb())
}
