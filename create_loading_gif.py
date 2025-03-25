import numpy as np
from PIL import Image, ImageDraw
import os

def create_loading_animation():
    """Создает анимированный GIF с загрузочным индикатором"""
    # Параметры анимации
    width, height = 150, 150
    center = width // 2, height // 2
    radius = 30
    dot_radius = 6
    dot_count = 8
    
    # Создаем кадры анимации
    frames = []
    for i in range(8):
        # Создаем пустое изображение с прозрачным фоном
        frame = Image.new('RGBA', (width, height), (0, 0, 0, 0))
        draw = ImageDraw.Draw(frame)
        
        # Рисуем точки загрузки
        for j in range(dot_count):
            # Угол для текущей точки
            angle = j * 2 * np.pi / dot_count
            
            # Координаты точки
            x = center[0] + int(radius * np.cos(angle))
            y = center[1] + int(radius * np.sin(angle))
            
            # Смещение относительно первого кадра
            offset = (i + j) % dot_count
            
            # Яркость точки в зависимости от смещения
            opacity = int(255 * (1 - offset / dot_count))
            
            # Рисуем точку
            draw.ellipse(
                (x - dot_radius, y - dot_radius, x + dot_radius, y + dot_radius), 
                fill=(255, 255, 255, opacity)
            )
        
        frames.append(frame)
    
    # Сохраняем анимацию в GIF
    frames[0].save(
        'loading.gif',
        save_all=True,
        append_images=frames[1:],
        duration=100,  # Время кадра в мс
        loop=0         # Бесконечное повторение
    )
    
    print("Анимация загрузки создана: loading.gif")

if __name__ == "__main__":
    create_loading_animation() 