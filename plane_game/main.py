#-*- coding=utf-8 -*-
#!/usr/bin/python

import os
import time
import pygame
from pygame.locals import *
from random import randint

import stars 
from game import *

#默认星空的速度
default_stars_speed = (50, 300)
#子弹种数和当前子弹ID,以及对应的子弹大小、发射频率(个/秒)
SHOT_NUM = 4
shot_id  = 1
shot_size= [(2,9), (16, 16), (19,14), (99,120)]
shot_rate= [ 0.15, 0.3, 0.10, 0.7 ]

dest_area = [\
(0,0,104,135), (104,0,104,135), (208,0,104,135),(312,0,104,135),(416,0,104,135),\
(0,135,102,112),(102,135,102,112),(204,135,102,112),(306,135,102,112),(408,135,102,112),\
(0,247,108,144),(108,247,100,144),(208,247,102,144),(310,247,100,144),(412,247,98,144),\
(0,400,95,100) ]

star_bmg = []
def update_background(stars, screen, time_passed_seconds):
    '''在指定图像上生成一些星星并移动，绘制'''
    stars.create_star(2,(randint(0,255),randint(0,255),randint(0,255)))
    #stars.create_star(1,(255,255,255))
    stars.move(time_passed_seconds)
    screen.fill((0, 0, 0))
    stars.draw(screen) 
    #screen.blit(star_bmg[0],(100,100))
    #screen.blit(star_bmg[1],(100,100))

def load_image(file, alpha=False):
    '''加载一张图片，可指定是否为alpha转换'''
    file = 'data/image/' + file
    try:
        surface = pygame.image.load(file)
    except pygame.error:
        raise SystemExit('加载图像 "%s" 失败 %s' % (file, pygame.get_error()) )
    if alpha:
        return surface.convert_alpha()
    return surface.convert()

def load_sound(file):
    file = 'data/music/' + file
    try:
        sound = pygame.mixer.Sound(file)
        return sound
    except pygame.error:
        print ('加载音乐 "%s" 失败' % file)
    return None

def main():
    global shot_id 
    global star_bmg

    pygame.mixer.pre_init(44100, -16, 2, 4096)
    pygame.init()
    screen = pygame.display.set_mode((800, 600)) #, FULLSCREEN)
 
    #加载各种资源数据
    image_list        = os.listdir('data/image')
    image_list.sort()
    Player.images     = [ load_image(file,True) for file in image_list if 'player' in file  ]
    Alien.images      = [ pygame.transform.rotate(load_image(file,True),180)\
                            for file in image_list if 'alien' in file  ]
    Shot.images       = [ load_image(file,True) for file in image_list if 'shot'   in file  ]
    star_bmg          = [ load_image(file,True) for file in image_list if 'star' in file ]

    Shot.shot_size    = shot_size
    shot_sound        = load_sound('shot2.wav')
    Explosion.images  = [ load_image(file,True) for file in image_list if 'explosion' in file ]
    explosion_sound1  = load_sound('explosion1.wav')
    explosion_sound2  = load_sound('explosion2.wav')
    change_shot_sound = load_sound('change_shot.wav')
    alarm_sound       = load_sound('alarm.wav')
    #加载并播放BGM
    pygame.mixer.music.load('data/music/bgm01.ogg')
    pygame.mixer.music.play(-1)

    # 初始化并生成一些星星
    world = stars.Stars(200)
    world.set_min_speed(default_stars_speed[0])
    world.set_max_speed(default_stars_speed[1])

    #为各种游戏对象分组,all组存储了所有游戏对象
    shots = pygame.sprite.Group()  #玩家的子弹和敌人的子弹分成2组
    alien_shots = pygame.sprite.Group() 
    aliens= pygame.sprite.Group()
    explosions = pygame.sprite.Group()
    all   = pygame.sprite.Group()

    Player.containers = all
    Alien.containers = aliens, all
    Shot.containers   = shots, all
    AlienShot.containers = alien_shots, all
    Explosion.containers = explosions, all

    player = Player()

    #玩家生命数，重载标志和重载时间
    life   = 3
    reloading = False
    reloading_time = 1.5

    Score.score = 0
    Score.life  = life
    score = Score()
    all.add(score)
    #无敌标志，重生后需要进入一段无敌时间
    iamyourdaddy = False

    clock = pygame.time.Clock()
    prev_time = 0.0
     
    while life or len(explosions.sprites())>0:
        allkill=None 
        for event in pygame.event.get():
            if event.type == QUIT:
                return
            if event.type == KEYDOWN and event.key == K_ESCAPE:
                return
            if event.type == KEYDOWN:
                #处理子弹切换
                if event.key == K_TAB:
                    shot_id = shot_id % SHOT_NUM + 1
                    change_shot_sound.play()
                elif event.key == K_x:
                    for alien in aliens:
                        alien.kill()
                        explosion_sound2.play()
                        Explosion(alien.rect.topleft)
                    for shot in alien_shots:
                        shot.kill()
                        explosion_sound2.play()
                        Explosion(shot.rect.topleft)

        keystate = pygame.key.get_pressed()
        time_passed = clock.tick(30)
        time_passed_seconds = time_passed / 1000.

        update_background(world, screen, time_passed_seconds)
        #all.clear(screen, screen)
        all.update(time_passed_seconds)

        #处理方向控制
        direct = []
        if keystate[K_UP]:
            direct.append('up')
            #模拟加速星空
            world.set_min_speed(default_stars_speed[0] * 10)
            world.set_max_speed(default_stars_speed[1] * 2)
        if keystate[K_DOWN]:
            direct.append('down')
            #模拟减速星空
            world.set_min_speed(10)
            world.set_max_speed(default_stars_speed[1] / 2)
        if keystate[K_LEFT]:
            direct.append('left')
        if keystate[K_RIGHT]:
            direct.append('right')
        player.move(direct)
        #若不是上下则恢复默认速度
        if  not (keystate[K_UP] or keystate[K_DOWN]):
            world.set_min_speed(default_stars_speed[0])
            world.set_max_speed(default_stars_speed[1])

        #处理攻击行为,用攻击间隔控制频率
        if  not reloading and keystate[K_SPACE]:
            if time.time()-prev_time > shot_rate[shot_id-1]:
                #第二个参数为射出角度，以12点钟方向为0度逆时针变大
                #Shot(player.attack_pos(), 45, shot_id)
                if shot_id==1:
                    SectorShot(player.attack_pos(), shot_id)
                else:
                    CommonShot(player.attack_pos(), shot_id)
                shot_sound.play()
                #Explosion(player.attack_pos())
                prev_time = time.time()

        #随机生成敌人，不同敌人血量不同
        n = randint(0,100)
        if n==1: 
            Alien(1,3)
        elif n==2:
            Alien(2,5)
        elif n==3:
            Alien(3,5)

        #处理玩家子弹与敌方的碰撞,碰撞字典键为第一个组的对象，值为第二个组的对象列表
        collide_dict = pygame.sprite.groupcollide(aliens,shots,False,False)
        for alien in collide_dict:
            for shot in collide_dict[alien]:
                if shot_id!=4:
                    shot.kill()
                explosion_sound1.play()
                harm = shot.harm
                if not alien.shoted_and_live(harm):
                    Score.score += 1
                    alien.kill()
                    explosion_sound2.play()
                    Explosion(alien.rect.topleft)

        #检测无敌时间是否结束
        if iamyourdaddy:
            wait += time_passed_seconds
            if wait > 1.5:
                iamyourdaddy = False
                wait = 0.0

        #如果玩家处于重生中则不检测玩家碰撞
        if not reloading:
            #处理玩家与敌人的碰撞
            for alien in pygame.sprite.spritecollide(player, aliens,True):
                explosion_sound2.play()
                Explosion(alien.rect.topleft)
                if iamyourdaddy:
                    pass
                else:
                    alarm_sound.play(2)

                    Explosion(player.rect.topleft)
                    Score.score += 1
                    Score.life  -= 1
                    player.kill()
                    reloading = True
                    wait = 0.0
                    life -= 1

        if not reloading:
            #处理玩家与敌方子弹的碰撞
            for shot in pygame.sprite.spritecollide(player, alien_shots, True):
                explosion_sound1.play()
                harm = shot.harm
                if iamyourdaddy:
                    pass
                elif not player.shoted_and_live(harm):
                    alarm_sound.play(2)

                    explosion_sound2.play()
                    Explosion(player.rect.topleft)
                    Score.life  -= 1
                    player.kill()
                    reloading = True
                    wait = 0.0
                    life -= 1

        #处理子弹与子弹的碰撞
        if shot_id==4:
            collide_dict = pygame.sprite.groupcollide(alien_shots,shots,True,False)
            for alien_shot in collide_dict:
                explosion_sound2.play()
                Explosion(alien_shot.rect.topleft)

        #死亡后重置玩家，生命数-1
        if reloading:  
            wait += time_passed_seconds 
            if wait > reloading_time: 
                reloading = False 
                player = Player()
                wait = 0.0
                #进入无敌模式
                iamyourdaddy = True

        # 增加一颗新的星星
        #stars.create_star(1)
        #stars.move(time_passed_seconds)
        #screen.fill((0, 0, 0))
        # 绘制所有的星
        #stars.draw(screen)
        #screen.blit(image,(300,300))
        all.draw(screen)
        pygame.display.update()

    #绘制结束画面
    #设置字体
    font = pygame.font.SysFont("文泉驿点阵正黑", 80)
    end  = font.render(u"YOU LOST!!!", True, (255,0,0))
    screen.blit(end, (180, 270))
    pygame.display.update()
    time.sleep(2.5)
 
if __name__ == "__main__":
    main()
