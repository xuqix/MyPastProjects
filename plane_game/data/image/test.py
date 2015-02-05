import pygame
from pygame.locals import *
 areas  = [\
(0,0,104,135), (104,0,104,135), (208,0,104,135),(312,0,104,135),(416,0,104,135),\
(0,135,102,112),(102,135,102,112),(204,135,102,112),(306,135,102,112),(408,135,102,112),\
(0,247,108,144),(108,247,100,144),(208,247,102,144),(310,247,100,144),(412,247,98,144),\
(0,400,95,100) ]
pygame.init()
screen = pygame.display.set_mode((640,480))

image = pygame.image.load('explosion2.tga').convert_alpha()
screen.fill((255,255,255))
screen.blit(image.subsurface(Rect(areas[5])), (100,100), (0,0,104,135))
pygame.display.update()

