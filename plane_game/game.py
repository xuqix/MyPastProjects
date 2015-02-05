#-*- coding=utf-8 -*-
#!/usr/bin/python

import pygame
from pygame.locals import *
from random import randint
#from gameobjects import vector2
import math
import time

SCREEN_RECT = pygame.Rect(0,0,800,600)

class Player(pygame.sprite.Sprite):
    '''玩家类'''
    speed  = 10
    images = []
    def __init__(self):
        pygame.sprite.Sprite.__init__(self, self.containers)
        self.image = Player.images[0]
        self.rect  = self.image.get_rect(midbottom=SCREEN_RECT.midbottom)
        self.health= 4
        #self.time  = 0
        #self.reloading = False

    #def update(self, time_passed_seconds=0.0): if not self.reloading: super(Player,self).update(time_passed_seconds) else: self.time += time_passed_seconds if self.time > 1.5: print self.time self.time = 0.0 self.reloading = False self.groups()[0].remove(self)

    def move(self, directions):
        '''移动,direction == 'up' or 'down' or 'left' or 'right' '''
        for direction in directions:
            if direction == 'up':
                self.rect.move_ip(0, -1 * Player.speed)
            elif direction == 'down':
                self.rect.move_ip(0, 1 * Player.speed)
            elif direction == 'left':
                self.rect.move_ip(-1 * Player.speed, 0)
            elif direction == 'right':
                self.rect.move_ip(1 * Player.speed, 0)
            else:
                print 'argument error'
                return None
        self.rect.clamp_ip(SCREEN_RECT)

    def shoted_and_live(self, harm):
        '''被攻击处理，依然存活返回True,否则返回False'''
        self.health -= harm
        if self.health <= 0:
            return False
        else:
            return True

    def attack_pos(self):
        return self.rect.x + self.rect.width / 2, self.rect.y

class Shot(pygame.sprite.Sprite):
    '''通用子弹类'''
    speed_tab = [ 13, 13, 26, 30 ]
    #子弹攻击力表
    harm_tab  = [  1,  2 , 3, 12]
    images    = []
    #子弹大小表
    shot_size = []
    def __init__(self, pos, angle, id=1 ):
        '''pos为射出位置
           angle参数为子弹射出的方向角度，以12点钟方向为0度，逆时针增大'''
        pygame.sprite.Sprite.__init__(self, self.containers)
        self.id    = id
        self.angle = angle
        self.speed = Shot.speed_tab[id-1]
        self.harm  = Shot.harm_tab[id-1]
        self.image = pygame.transform.scale(Shot.images[id-1], Shot.shot_size[id-1])
        self.image = pygame.transform.rotate(self.image, angle)
        self.rect  = self.image.get_rect(midbottom=pos)

    def update(self,time_passed_seconds=0.0):
        radian = self.angle / 180.0 * math.pi
        self.rect.move_ip(math.sin(radian) * -self.speed,\
                -self.speed * math.cos(radian) )
        if self.rect.x+self.rect.width < 0 or\
                self.rect.x > SCREEN_RECT.width or\
                self.rect.y+self.rect.height < 0 or\
                self.rect.y > SCREEN_RECT.height:
            self.kill()

class AlienShot(Shot):
    '''
        敌方子弹类
        为了对象分组专为敌人的子弹使用一个新类,并定制子弹射速
    '''
    def __init__(self, pos, angle, id=1, speed=20 ):
        Shot.__init__(self, pos, angle, id)
        self.speed = speed

def SectorShot(pos, shot_id):
    '''扇形子弹的封装'''
    Shot(pos, 0,  shot_id)
    Shot(pos, 15, shot_id)
    Shot(pos, 30, shot_id)
    Shot(pos, 45, shot_id)
    Shot(pos, 315,shot_id)
    Shot(pos, 330,shot_id)
    Shot(pos, 345,shot_id)


def CommonShot(pos, shot_id):
    '''常规子弹'''
    Shot(pos, 0, shot_id)

class Alien(pygame.sprite.Sprite):
    '''通用敌人类'''
    #默认速度表，速度为像素/秒
    speed_tab = [ 400, 200, 200 ]
    images= []
    #用于射击间隔
    times = [0.15, 0.3, 0.4]
    
    def __init__(self, id=1, health=5):
        pygame.sprite.Sprite.__init__(self, self.containers)
        self.id     = id
        self.speed  = Alien.speed_tab[id-1]
        self.health = health
        self.image  = Alien.images[id-1]
        self.rect   = self.image.get_rect()
        self.rect.topleft = (randint(0, SCREEN_RECT.width-self.rect.width),0)

        self.move_tab  = [ self.move_line, self.move_circle, self.move_curve ]
        #用于射击的时间计算
        self.time   = 0.0

    def update(self, time_passed_seconds=0.0):
        self.move_tab[self.id-1](time_passed_seconds)
        if self.rect.x < 0 or self.rect.x > SCREEN_RECT.width or self.rect.y < 0 or self.rect.y > SCREEN_RECT.height:
            self.kill()
        self.time += time_passed_seconds
        if self.time > Alien.times[self.id-1]:
            self.time = 0.0
            if self.id == 1:
                AlienShot(self.attack_pos(), 180, 1, 30)
            elif self.id == 2:
                AlienShot(self.attack_pos(), 120, 1, 10)
                AlienShot(self.attack_pos(), 150, 1, 10)
                AlienShot(self.attack_pos(), 180, 1, 10)
                AlienShot(self.attack_pos(), 210, 1, 10)
                AlienShot(self.attack_pos(), 240, 1, 10)
            elif self.id == 3:
                AlienShot(self.attack_pos(), 180, 2, 10)


    def shoted_and_live(self, harm):
        '''被攻击处理，依然存活返回True,否则返回False'''
        self.health -= harm
        if self.health <= 0:
            return False
        else:
            return True

    def move_line(self, time_passed_seconds):
        self.rect.move_ip(0, self.speed * time_passed_seconds)

    def move_circle(self, time_passed_seconds):
        if not hasattr(self, 'angle'):
            self.angle = 180
        else:
            self.angle = self.angle+time_passed_seconds*360
        if not hasattr(self, 'radius'):
            self.radius = 60
        if not hasattr(self, 'center'):
            x = self.rect.x+self.radius if self.rect.x < self.radius else self.rect.x-self.radius
            self.center = [ x, 0+self.radius]
        self.center[1] += 2
        new_pos = self.__circle_next( self.center, self.radius, self.angle) 
        #self.rect.move_ip(new_pos[0], new_pos[1])
        self.rect.x, self.rect.y = new_pos[0], new_pos[1]

    def __circle_next(self, center, radius, angle):
        x = math.sin(angle/180.0*math.pi) * radius + center[0]
        y = math.cos(angle/180.0*math.pi) * radius + center[1]
        return x, y

    def move_curve(self, time_passed_seconds):
        if not hasattr(self, 'ray'):
            self.ray = self.rect.x
        if not hasattr(self, 'angle'):
            self.angle = 0
        else:
            self.angle = self.angle + time_passed_seconds * 360
        if not hasattr(self, 'curve_width'):
            self.curve_width = 50
        x = math.sin(self.angle/180*math.pi) * self.curve_width + self.ray
        y = self.rect.y + self.speed * time_passed_seconds
        self.rect.x, self.rect.y = x, y

    def attack_pos(self):
        return self.rect.x + self.rect.width / 2, self.rect.y + self.rect.height
    
class Explosion(pygame.sprite.Sprite):
    '''爆炸类'''
    #用于存储爆炸图像每帧的坐标
    areas  = [\
(0,0,104,135), (104,0,104,135), (208,0,104,135),(312,0,104,135),(416,0,94,135),\
(0,135,102,112),(102,135,102,112),(204,135,102,112),(306,135,102,112),(408,135,102,112),\
(0,247,108,144),(108,247,100,144),(208,247,102,144),(310,247,100,144),(412,247,98,144),\
(0,400,95,100) ]
    images = []
    
    def __init__(self, pos, id=1, areas=None):
        pygame.sprite.Sprite.__init__(self, self.containers)
        self.pos = pos
        self.fps = 0
        self.image_data = Explosion.images[id-1]
        if areas is not None:
            self.areas = areas

        self.update()


    def update(self, time_passed_seconds=0.0):
        self.rect  = pygame.Rect(self.areas[self.fps])
        self.image = self.image_data.subsurface(Rect(self.areas[self.fps]))
        self.rect.topleft = self.pos
        self.fps += 1
        if self.fps >= len(self.areas):
            self.kill()

class Score(pygame.sprite.Sprite):

    score = 0
    health= 0
    life  = 0
    def __init__(self, font_type = "文泉驿点阵正黑"):
        pygame.sprite.Sprite.__init__(self)
        self.font =  pygame.font.SysFont(font_type, 20)
        self.color=  (255,255,255)
        self.msg  =  u"得分：%d\n生命：%d"
        self.update()
        self.rect =  self.image.get_rect()
        self.rect.topleft = (10,10)

    def update(self, time_passed_seconds=0.0):
        self.msg = u"生命：%d   得分：%d"% (Score.life, Score.score)
        self.image = self.font.render(self.msg, True, self.color)


