Moodlamp scripting
---------
Prismatik uses [QJSEngine](https://doc.qt.io/qt-5/qjsengine.html) for parsing and running moodlamp scripts, so if you are familiar with JavaScript you'll be fine.

The scripts live in your user's Prismatik folder:
- Windows: `C:\Users\<you>\Prismatik\moodlamps`
- macOS & Linux: `~/.Prismatik/moodlamps`

File name should obey these rules:
- allowed characters `a-z`, `0-9` and `_` (underscore)
- `.mjs` extension
- unique name

For ex: `my_cool_lamp.mjs`


**The bare minimum template:**
```js
export const name = "your lamp name here"
export const interval = 50 // frame time in ms

/**
 * @param {integer}	baseColor	integer RGB value of the base color that is selected in Prismatik
 * @param {array}	colors		array of RGB integers representing your LEDs
 * @return {array}	array of new RGB integers representing your LEDs
 */
export function tick(baseColor, colors)
{
	for (let i = 0; i < colors.length; i++)
	{
		// your code here, for ex:
		colors[i] = baseColor // will set all LEDs to the color you set in Prismatik
	}

	return colors
}
```


**More examples:**

```js
export const name = "your lamp name here"
export const interval = 50 // frame time in ms

// if you need a global constant
const myBlueConst = 0x0000FF // blue color

// if you need a variable that persists between frames (a counter for ex), declare it here
let someVariable = 0

export function tick(baseColor, colors)
{
	for (let i = 0; i < colors.length; i++)
	{
		// to set all your LEDs to white
		colors[i] = 0xFFFFFF
		// or to blue
		colors[i] = myBlueConst

		// random colors for each LED
		colors[i] = Math.round(Math.random() * 0xFFFFFF)
	}

	someVariable++ // counter from above

	return colors
}

```

**Recommended way:**

For convenience, Prismatik partially exposes `QColor` class, see [here for documentation](https://doc.qt.io/qt-5/qcolor.html) and [here for exposed methods](/Software/src/QColorMetaWrapper.hpp).
Prismatik uses `QColor` internally so for best compatibility it's better to use this class.
Raw RGB values should work, but use them at your own risk.

Example from `rgb_is_life.js`:
```js
export const name = "RGB is Life"
export const interval = 33 // ms

const speed = 1.5

let m_frames = 0

export function tick(baseColor, colors)
{
	// transform base RGB value into QColor
	baseColor = new QColor(baseColor)

	// RGB transition math
	const degrees = 360.0 / colors.length
	const step = speed * m_frames++
	for (let i = 0; i < colors.length; i++)
	{
		// transform current color for a given LED into QColor
		let color = new QColor(colors[i])

		// increment baseColor's hue value for that smooth RGB rotation
		const hue = baseColor.hslHue() + degrees * i + step

		// set new hue while preserving baseColor's saturation and lightness
		color.setHsl(hue, baseColor.hslSaturation(), baseColor.lightness())

		// assign the new color
		// colors[] has to contain .rgb() values
		colors[i] = color.rgb()
	}
	if (step > 360)
		m_frames = 0

	return colors
}
```


To see your changes use the reload button or relaunch the app.


**Warnig:** scripts bundled with Prismatik are overwritten on launch, if you want to customize Prismatik's scripts, make sure to make a copy of them and modify those.


**Logging and console**

If you enable logs in Prismatik your JS errors will be logged to
- Windows: `C:\Users\<you>\Prismatik\Logs`
- macOS & Linux: `~/.Prismatik/Logs`


You can also enable the [Console API](https://doc.qt.io/qt-5/qtquick-debugging.html#console-api)

```js
export const enableConsole = true
...
export function tick()
{
	...
	console.log("some debug")
	...
}
```

Don't forget to remove when you are done
