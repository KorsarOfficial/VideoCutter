import os
import subprocess
import tempfile
import shutil
import re
import cv2
from pathlib import Path
from PyQt5.QtCore import QObject, pyqtSignal

class VideoProcessor(QObject):
    # Сигнал прогресса обработки (0-100)
    progress_updated = pyqtSignal(int)
    
    def __init__(self):
        super().__init__()
        self.current_video = None
        self.duration = 0
        self.total_frames = 0
        self.fps = 0
        
        # Проверяем наличие FFmpeg
        self._check_ffmpeg()
        
    def _check_ffmpeg(self):
        """Проверяет наличие FFmpeg в системе"""
        try:
            subprocess.run(
                ["ffmpeg", "-version"], 
                stdout=subprocess.PIPE, 
                stderr=subprocess.PIPE,
                check=True
            )
        except (subprocess.SubprocessError, FileNotFoundError):
            print("ВНИМАНИЕ: FFmpeg не найден. Установите FFmpeg для работы с приложением.")
            print("Скачать можно здесь: https://ffmpeg.org/download.html")
    
    def load_video(self, file_path):
        """Загружает видео и получает информацию о нем"""
        if os.path.exists(file_path):
            self.current_video = file_path
            
            # Получаем информацию о видео через ffprobe
            command = [
                'ffprobe', 
                '-v', 'error',
                '-show_entries', 'format=duration',
                '-of', 'default=noprint_wrappers=1:nokey=1',
                file_path
            ]
            
            result = subprocess.run(command, capture_output=True, text=True)
            try:
                self.duration = float(result.stdout.strip())
            except ValueError:
                self.duration = 0
            
            # Получаем FPS видео
            command = [
                'ffprobe',
                '-v', 'error',
                '-select_streams', 'v:0',
                '-show_entries', 'stream=r_frame_rate',
                '-of', 'default=noprint_wrappers=1:nokey=1',
                file_path
            ]
            
            result = subprocess.run(command, capture_output=True, text=True)
            try:
                # FPS может быть представлен как дробь (например, 30000/1001)
                fps_str = result.stdout.strip()
                if '/' in fps_str:
                    numerator, denominator = map(int, fps_str.split('/'))
                    self.fps = numerator / denominator
                else:
                    self.fps = float(fps_str)
                
                # Вычисляем общее количество кадров
                self.total_frames = int(self.duration * self.fps)
            except (ValueError, ZeroDivisionError):
                self.fps = 0
                self.total_frames = 0
            
            return True
        return False
    
    def cut_video(self, start_time, end_time, output_file, output_format="mp4"):
        """Обрезает видео с указанного начального времени до конечного времени"""
        if not self.current_video or not os.path.exists(self.current_video):
            return False
        
        # Проверяем формат вывода и устанавливаем расширение файла
        format_to_ext = {
            "MP4": ".mp4",
            "AVI": ".avi",
            "MOV": ".mov",
            "MKV": ".mkv",
            "WebM": ".webm",
            "OGG": ".ogg",
            "FLV": ".flv",
            "WMV": ".wmv"
        }
        
        # Добавляем расширение, если оно не указано
        file_ext = format_to_ext.get(output_format.upper(), ".mp4")
        if not output_file.lower().endswith(file_ext.lower()):
            output_file += file_ext
        
        # Создаем временный файл для вывода
        temp_output = tempfile.NamedTemporaryFile(suffix=file_ext, delete=False)
        temp_output.close()
        
        # Формируем команду для FFmpeg с копированием аудио и видео потоков
        command = [
            'ffmpeg',
            '-i', self.current_video,  # Входной файл
            '-ss', str(start_time),    # Начальное время
            '-to', str(end_time),      # Конечное время
            '-map', '0',               # Копируем все потоки
            '-c', 'copy',              # Кодек (copy = без перекодирования)
            temp_output.name           # Выходной файл
        ]
        
        try:
            # Запускаем процесс
            process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
            
            # Отслеживаем прогресс
            duration = end_time - start_time
            
            # Читаем вывод процесса
            for line in process.stderr:
                if "time=" in line:
                    # Извлекаем текущее время из вывода FFmpeg
                    time_match = re.search(r'time=(\d+):(\d+):(\d+\.\d+)', line)
                    if time_match:
                        hours, minutes, seconds = map(float, time_match.groups())
                        current_time = hours * 3600 + minutes * 60 + seconds
                        progress = int((current_time / duration) * 100)
                        # Ограничиваем прогресс до 100%
                        progress = min(progress, 100)
                        self.progress_updated.emit(progress)
            
            # Ждем завершения процесса
            process.wait()
            
            # Проверяем успешность выполнения
            if process.returncode == 0:
                # Копируем временный файл в указанное место
                shutil.move(temp_output.name, output_file)
                return True
            else:
                # Удаляем временный файл в случае ошибки
                os.remove(temp_output.name)
                return False
                
        except Exception as e:
            print(f"Ошибка при обрезке видео: {e}")
            if os.path.exists(temp_output.name):
                os.remove(temp_output.name)
            return False
    
    def extract_frame(self, timestamp):
        """Извлекает кадр из видео в указанный момент времени и возвращает путь к файлу"""
        if not self.current_video or not os.path.exists(self.current_video):
            return None
        
        # Создаем временный файл для хранения кадра
        temp_frame = tempfile.NamedTemporaryFile(suffix=".jpg", delete=False)
        temp_frame.close()
        
        # Формируем команду для FFmpeg
        command = [
            'ffmpeg',
            '-ss', str(timestamp),  # Временная метка
            '-i', self.current_video,  # Входной файл
            '-vframes', '1',  # Извлекаем только один кадр
            '-q:v', '2',  # Качество (2 - высокое)
            temp_frame.name  # Выходной файл
        ]
        
        try:
            # Запускаем процесс и ждем его завершения
            subprocess.run(command, capture_output=True, check=True)
            return temp_frame.name
        except subprocess.CalledProcessError:
            # В случае ошибки удаляем временный файл и возвращаем None
            if os.path.exists(temp_frame.name):
                os.remove(temp_frame.name)
            return None
    
    def _get_format_params(self, output_format):
        """Возвращает параметры кодирования для указанного формата"""
        formats = {
            "mp4": {
                "video_codec": "libx264",
                "audio_codec": "aac",
                "extra_params": ["-preset", "medium"]
            },
            "avi": {
                "video_codec": "mpeg4",
                "audio_codec": "libmp3lame"
            },
            "mov": {
                "video_codec": "libx264",
                "audio_codec": "aac"
            },
            "mkv": {
                "video_codec": "libx264",
                "audio_codec": "libvorbis"
            },
            "webm": {
                "video_codec": "libvpx-vp9",
                "audio_codec": "libopus",
                "extra_params": ["-b:v", "1M", "-b:a", "128k"]
            },
            "ogg": {
                "video_codec": "libtheora",
                "audio_codec": "libvorbis"
            },
            "flv": {
                "video_codec": "flv",
                "audio_codec": "libmp3lame"
            },
            "wmv": {
                "video_codec": "wmv2",
                "audio_codec": "wmav2"
            }
        }
        
        return formats.get(output_format.lower(), {
            "video_codec": "libx264",
            "audio_codec": "aac"
        })
    
    def format_time(self, seconds):
        """Форматирует время в формат HH:MM:SS"""
        hours = int(seconds) // 3600
        minutes = (int(seconds) % 3600) // 60
        secs = int(seconds) % 60
        msecs = int((seconds - int(seconds)) * 100)
        return f"{hours:02d}:{minutes:02d}:{secs:02d}.{msecs:02d}" 