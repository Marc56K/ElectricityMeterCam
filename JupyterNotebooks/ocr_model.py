import os
import time
from PIL import Image, ImageEnhance, ImageOps
import random
from random import randint
import matplotlib.pyplot as plt
import numpy as np
from tqdm import *
from tensorflow.keras.utils import to_categorical
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense, Conv2D, Flatten, MaxPooling2D, Dropout
import tensorflow as tf;
from keras.preprocessing.image import ImageDataGenerator
from training_set_utils import *
from image_utils import *

class OcrModel:
    cnn = None
    input_width = 28
    input_height = 28
    output_classes = 11
    
    def __init__(self):
        self.cnn = Sequential()
        self.cnn.add(Conv2D(16, (3, 3), activation = 'relu', input_shape = (self.input_height, self.input_width, 1)))
        self.cnn.add(MaxPooling2D(2, 2))
        self.cnn.add(Dropout(0.5))
        self.cnn.add(Conv2D(32, (3, 3), activation = 'relu'))
        self.cnn.add(MaxPooling2D(2, 2))
        self.cnn.add(Dropout(0.5))
        self.cnn.add(Conv2D(32, (3, 3), activation = 'relu'))
        self.cnn.add(Flatten())
        self.cnn.add(Dense(64, activation = 'relu'))
        self.cnn.add(Dropout(0.5))
        self.cnn.add(Dense(self.output_classes, activation = 'softmax'))
        self.cnn.compile(optimizer="rmsprop", loss="categorical_crossentropy", metrics=["acc"])
        #self.cnn.summary()
        
    def load_weights(self):
        files = os.listdir("weights")    
        files = [file for file in files if file[-3:] == ".h5"]
        files.sort(reverse=True)
        for f in files:
            if f.find("c" + str(self.output_classes)) != -1:
                print("loading " + f)
                self.cnn.load_weights("weights/" + f)
                break
    
    def save_weights(self, info = None):
        fname = str(int(time.time())) + "_weights_" + str(self.input_width) + "x" + str(self.input_height) + "_c" + str(self.output_classes)
        if info != None:
            fname = fname + "_" + info
        fname = fname + ".h5"
        self.cnn.save_weights("weights/" + fname)
        
    def export_as_tflite(self):
        converter = tf.lite.TFLiteConverter.from_keras_model(self.cnn)
        tflite_model = converter.convert()
        filename = "ocr_model_" + str(self.input_width) + "x" + str(self.input_height) + "_c" + str(self.output_classes)
        print("writing " + filename + ".tflite")
        open(filename + ".tflite", "wb").write(tflite_model)
        print("TODO call in git bash: xxd -i " + filename + ".tflite > ../src/" + filename + ".c")
        
    def image_to_nparray(self, image):
        image = image.convert("L")
        image = image.resize((self.input_width, self.input_height), Image.LANCZOS)
        return np.asarray(image).astype(np.float32) / 255.0

    def convert_data_set(self, data_set):
        x_arr = []
        y_arr = []
        for i, item in enumerate(tqdm(data_set)):
            x_arr.append(self.image_to_nparray(item.image))
            y_arr.append(np.asarray(item.y).astype(np.float32))

        # convert to numpy types
        x = np.asarray(x_arr)
        x = x.reshape(x.shape[0], x.shape[1], x.shape[2], 1)        
        y = np.asarray(y_arr)

        return (x, y)
    
    def predict(self, image):
        x = self.image_to_nparray(image)
        return self.cnn.predict(x.reshape(1, x.shape[0], x.shape[1], 1))[0]