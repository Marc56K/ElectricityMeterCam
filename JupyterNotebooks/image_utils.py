from PIL import Image, ImageEnhance, ImageOps, ImageChops

def get_fillcolor(image):
    a = image.getpixel((0, 0))
    b = image.getpixel((image.width - 1, image.height - 1))
    if len(image.getbands()) == 1:
        return int((a + b) / 2)
    else:
        c = []
        for i in range(len(a)):
            c.append(int((a[i] + b[i]) / 2))
        return tuple(c)
        

def img_brightness(image, b):
    img = ImageEnhance.Brightness(image).enhance(b)        
    return img

def img_contrast(image, c):
    img = ImageEnhance.Contrast(image).enhance(c)        
    return img

def img_horizontal_shift(image, offset):
    img = None
    col = get_fillcolor(image)
    if offset > 0:
        img = image.crop((0, 0, 1, image.height))
    else:
        img = image.crop((image.width - 1, 0, image.width, image.height))
    img = img.resize(image.size)
    img.paste(image, (int(offset * image.width), 0))
    return img

def img_rotate(image, angle):
    col = get_fillcolor(image)
    img = image.rotate(angle=angle, fillcolor=col)
    return img

def img_innerscale(image, x_scale, y_scale):
    col = get_fillcolor(image)
    w = int(round(image.width * x_scale))
    h = int(round(image.height * y_scale))
    img =  Image.new(image.mode, image.size, col)
    x = int(round(image.width / 2 - w / 2))
    y = int(round(image.height / 2 - h / 2))
    img.paste(image.resize((w, h)), (x, y))
    return img

def img_noise(image, sd = 20, brightness = 2.0):
    noise_img = Image.effect_noise(image.size, sd).convert(mode=image.mode)
    img = ImageChops.multiply(image, noise_img)
    img = img_brightness(img, brightness)
    return img

def img_letterbox(image, w, h):
    col = get_fillcolor(image)
    return ImageOps.pad(image, (w, h), color = col)

def img_crop_or_pad(image, w, h):
    col = get_fillcolor(image)
    img =  Image.new(image.mode, (w, h), col)
    x = int(round((w - image.width) / 2))
    y = int(round((h - image.height) / 2))
    img.paste(image, (x, y))
    return img