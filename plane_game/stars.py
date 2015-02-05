#-*- coding=utf-8 -*-
#!/usr/bin/python


import pygame
from pygame.locals import *
from random import randint
import math
 
class Star(object):
    def __init__(self, x, y, speed, color=(255,255,255)):
 
        self.x = x
        self.y = y
        self.speed = speed
        self.color = color

class Stars(object):
    '''
        用于绘制星星背景
    '''
    def __init__(self, num = 0, SCREEN_SIZE=(800,600), color=(255,255,255)):
        self.stars = []
        self.MIN_SPEED = 10
        self.MAX_SPEED = 300
        self.SCREEN_SIZE = SCREEN_SIZE
        if num > 0:
            self.create_star(num, color)

    def set_min_speed(self,speed):
        self.MIN_SPEED = speed
    def set_max_speed(self,speed):
        self.MAX_SPEED = speed

    def create_star(self,num = 1, color = (255,255,255)):
        '''创建一个或多个星星,颜色可选'''
        for i in xrange(0,num):
            x = float(randint(0, self.SCREEN_SIZE[0]))
            y = float(randint(0, self.SCREEN_SIZE[1]))
            speed = float(randint(self.MIN_SPEED, self.MAX_SPEED))
            self.stars.append( Star(x, y, speed, color) )

    def move(self,time_passed_seconds):
        '''移动星星并过滤'''
        for star in self.stars:
            star.y = star.y + time_passed_seconds * star.speed
        #过滤跑出画面的星星
        self.stars = filter(lambda one: one.y<=self.SCREEN_SIZE[1], self.stars)

    def draw(self, surface):
        '''将星星画到指定图像对象'''
        for star in self.stars:
            #pygame.draw.aaline(surface, star.color,\
            #        (star.x, star.y), (star.x+1., star.y))
            surface.set_at((int(star.x),int(star.y)),star.color)
 

def test():
 
    pygame.init()
    screen = pygame.display.set_mode((800, 600)) #, FULLSCREEN)
 
    stars = Stars()
    #stars.set_max_speed(1000)
    #stars.set_min_speed(300)
 
    # 在第一帧，画上一些星星
    stars.create_star(200)
     
    clock = pygame.time.Clock()
 
    white = (255, 255, 255)
     
    while True:
 
        for event in pygame.event.get():
            if event.type == QUIT:
                return
            if event.type == KEYDOWN:
                return

        time_passed = clock.tick(30)
        time_passed_seconds = time_passed / 1000.
 
        #update_background(stars, screen, time_passed_seconds)
        # 增加一颗新的星星
        stars.create_star(1)
        stars.move(time_passed_seconds)
 
        screen.fill((0, 0, 0))
 
        # 绘制所有的星
        stars.draw(screen)
 
        pygame.display.update()
 
if __name__ == "__main__":
    test()
