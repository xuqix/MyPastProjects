import pygame
from pygame.locals import *
import time

pygame.init()
screen = pygame.display.set_mode((640,480))

image1 = pygame.image.load('spriteSheet1.png').convert_alpha()
image2 = pygame.image.load('spriteSheet2.png').convert_alpha()
image3 = image1.subsurface(pygame.Rect(0,254,128,264))
image4 = image1.subsurface(pygame.Rect(667,0,94,169))
image5 = image2.subsurface(pygame.Rect(264,0,94,169))
#image3 = pygame.transform.rotate(image2, 
print image4,image5
screen.fill((255,255,255))
screen.blit(image3, (100,100))
screen.blit(image4, (100-image3.get_rect().width+95,100+\
        image3.get_rect().height-image4.get_rect().height))
screen.blit(image5, (100+image3.get_rect().width-60,100+\
        image3.get_rect().height-image4.get_rect().height))

#pygame.image.save(image3,'x.png')
#image3.blit(image4, (-image3.get_rect().width+95,+\
#        image3.get_rect().height-image4.get_rect().height))
#image3.blit(image5, (+image3.get_rect().width-60,+\
#        image3.get_rect().height-image4.get_rect().height))
pygame.display.update()
time.sleep(2)
pygame.image.save(image3,'x.png')

