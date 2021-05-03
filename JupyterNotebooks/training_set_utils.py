import os
import json
import random
import matplotlib.pyplot as plt
from tqdm import *
from image_utils import *
from training_item import *
from random import randint
from PIL import Image, ImageEnhance, ImageOps

def create_from_atlas(filePath, w_digit, h_digit):
    image = Image.open(filePath)
    
    scale = h_digit / (image.height / 10.0)
    w_img = int(round(image.width * scale))
    h_img = int(round(image.height * scale))
    image = image.resize((w_img, h_img))
    image = img_crop_or_pad(image, w_digit, h_img)
    w_img = w_digit    

    comp = Image.new('L', (w_img, h_img + h_digit))
    comp.paste(image, (0, 0))
    comp.paste(image, (0, h_img))
    
    plt.imshow(comp, vmin=0, vmax=255)
    plt.show()

    training_set = []
    for i in range(h_img):
        item = TrainingItem()
        item.image = comp.crop((0, i, w_img, i + h_digit))

        upper_digit_f = (float(i) / h_img) * 10
        upper_digit = int(upper_digit_f)
        upper_f = 1.0 - (upper_digit_f - upper_digit)

        lower_digit_f = upper_digit_f + 1
        lower_digit = int(lower_digit_f) % 10
        lower_f = 1.0 - upper_f

        item.y[upper_digit] = upper_f
        item.y[lower_digit] = lower_f

        training_set.append(item)

    return training_set

def create_from_atlas_dir(dir_path, w_digit = 24, h_digit = 40):
    files = os.listdir(dir_path)    
    files = [file for file in files if file[-4:] == ".png"]
    training_set = []
    for file in files:
        try:
            training_set = training_set + create_from_atlas(os.path.join(dir_path, file), w_digit, h_digit)            
        except OSError:
            pass
    
    return training_set

def create_from_labeled_dir(dir_path):
    training_set = []
    for i in range(10):
        path = os.path.join(dir_path, str(i))
        if os.path.isdir(path):
            files = os.listdir(path)    
            files = [file for file in files if file[-4:] == ".png"]
            for j, file in enumerate(tqdm(files)):
                try:
                    item = TrainingItem()
                    item.image = Image.open(os.path.join(path, file))
                    item.y[i] = 1.0
                    training_set.append(item)
                except OSError:
                    pass
    
    return training_set

def create_variations(training_set):
    result = []
    for i, item in enumerate(tqdm(training_set)):
        img0 = item.image
        for shift in range(-25, 26, 5):
            hshift = shift / 100
            img1 = img_horizontal_shift(img0, hshift)
            for bright in range(4, 16, 2):
                b = bright / 10
                img2 = img_brightness(img1, b)
                for xscale in range(10, 15):
                    xs = xscale / 10                    
                    for yscale in range(8, 13):
                        ys = yscale / 10
                        img3 = img_innerscale(img2, xs, ys)
                        new_item = TrainingItem()
                        new_item.image = img3
                        new_item.y = item.y
                        result.append(new_item)
                        
                        img4 = img_noise(img3)
                        new_item = TrainingItem()
                        new_item.image = img4
                        new_item.y = item.y
                        result.append(new_item)

    return result

def load_from_captured(bboxesjson_file, images_dir):
    result = []
    with open(bboxesjson_file) as f:
        bboxes = json.load(f)
        files = os.listdir(images_dir)    
        files = [file for file in files if file[-4:] == ".jpg"]
        for i, file in enumerate(tqdm(files)):
            filePath = images_dir + "/" + file
            try:
                image = Image.open(filePath)
                for bb in bboxes:
                    digit_img = image.crop((bb["x"], bb["y"], bb["x"] + bb["w"], bb["y"] + bb["h"]))
                    result.append((digit_img, bb))
                    #digit_img.save(dstPath + "/x" + str(region[0]) + "_y" + str(region[1]) + "_" + file.replace(".jpg", ".png"))
            except OSError as err:
                print(str(err))
        
    return result