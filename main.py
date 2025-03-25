import sys
import os
import time
import datetime
import vlc
from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, 
                            QLabel, QPushButton, QFileDialog, QSlider, QComboBox, QFrame,
                            QProgressBar, QStyle, QSizePolicy, QDialog, QLineEdit, QGraphicsDropShadowEffect)
from PyQt5.QtCore import Qt, QTimer, QThread, pyqtSignal, QUrl, QMimeData, QPropertyAnimation, QRect, QEasingCurve, QPoint, QEvent
from PyQt5.QtGui import QPixmap, QFont, QColor, QPalette, QMovie, QDrag, QIcon, QPainter, QBrush, QPen, QCursor
from PyQt5.QtMultimedia import QMediaPlayer, QMediaContent
from PyQt5.QtMultimediaWidgets import QVideoWidget
import random
from video_processor import VideoProcessor
from create_loading_gif import create_loading_animation

class SplashScreen(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowFlags(Qt.FramelessWindowHint)
        self.setAttribute(Qt.WA_TranslucentBackground)
        self.setFixedSize(500, 500)
        
        # Центрирование окна на экране
        self.center()
        
        # Установка счетчика времени и прогресса
        self.counter = 0
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_progress)
        self.timer.start(40)  # 25 fps примерно
        
        # Тексты для анимированного набора
        self.made_by_full_text = "Made By"
        self.author_full_text = "Korsar Official"
        self.made_by_current_text = ""
        self.author_current_text = ""
        self.cursor_visible = True
        self.made_by_typing_finished = False
        self.author_typing_finished = False
        self.typing_delay = 0  # счетчик для задержки между символами
        
        # Создание разметки
        layout = QVBoxLayout()
        self.setLayout(layout)
        
        # Добавление логотипа
        logo_label = QLabel()
        logo_pixmap = QPixmap("logo/logo.jpg")
        logo_label.setPixmap(logo_pixmap.scaled(300, 300, Qt.KeepAspectRatio, Qt.SmoothTransformation))
        logo_label.setAlignment(Qt.AlignCenter)
        layout.addWidget(logo_label)
        
        # Добавление текста с анимацией Made By
        self.made_by_label = QLabel(self.made_by_current_text)
        self.made_by_label.setAlignment(Qt.AlignCenter)
        self.made_by_label.setStyleSheet("color: white; font-size: 22px; font-weight: bold;")
        layout.addWidget(self.made_by_label)
        
        # Добавление текста с анимацией Korsar Official
        self.author_label = QLabel(self.author_current_text)
        self.author_label.setAlignment(Qt.AlignCenter)
        self.author_label.setStyleSheet("color: white; font-size: 28px; font-weight: bold;")
        layout.addWidget(self.author_label)
        
        # Добавление вращающегося индикатора загрузки
        loading_label = QLabel()
        self.loading_movie = QMovie("loading.gif")
        loading_label.setMovie(self.loading_movie)
        loading_label.setAlignment(Qt.AlignCenter)
        self.loading_movie.start()
        layout.addWidget(loading_label)
        
    def center(self):
        qr = self.frameGeometry()
        cp = QApplication.desktop().availableGeometry().center()
        qr.moveCenter(cp)
        self.move(qr.topLeft())
        
    def update_progress(self):
        self.counter += 1
        
        # Анимация набора текста для "Made By"
        if not self.made_by_typing_finished:
            self.typing_delay += 1
            # Каждые 5 кадров добавляем новый символ
            if self.typing_delay >= 5:
                self.typing_delay = 0
                next_char_index = len(self.made_by_current_text)
                if next_char_index < len(self.made_by_full_text):
                    self.made_by_current_text += self.made_by_full_text[next_char_index]
                else:
                    self.made_by_typing_finished = True
        
        # Начинаем печатать "Korsar Official" только после завершения "Made By"
        if self.made_by_typing_finished and not self.author_typing_finished:
            self.typing_delay += 1
            # Каждые 5 кадров добавляем новый символ
            if self.typing_delay >= 5:
                self.typing_delay = 0
                next_char_index = len(self.author_current_text)
                if next_char_index < len(self.author_full_text):
                    self.author_current_text += self.author_full_text[next_char_index]
                else:
                    self.author_typing_finished = True
        
        # Мигание курсора (каждые 15 кадров)
        if self.counter % 15 == 0:
            self.cursor_visible = not self.cursor_visible
        
        # Обновляем текст с курсором
        cursor = "▎" if self.cursor_visible else ""
        
        if not self.made_by_typing_finished:
            self.made_by_label.setText(self.made_by_current_text + cursor)
        else:
            self.made_by_label.setText(self.made_by_current_text)
            
        if self.made_by_typing_finished and not self.author_typing_finished:
            self.author_label.setText(self.author_current_text + cursor)
        elif self.author_typing_finished:
            self.author_label.setText(self.author_current_text)
        
        # Когда счетчик достигает 100 (примерно 4 секунды), запускаем основное приложение
        if self.counter >= 140:  # Увеличиваем, чтобы текст успел напечататься
            self.timer.stop()
            self.loading_movie.stop()
            self.close()
            
            # Запуск основного приложения
            self.main_window = MainWindow()
            self.main_window.show()

class StarBackground(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.stars = []
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update)
        self.timer.start(50)  # Обновление каждые 50мс
        
        # Генерация начальных звезд
        for _ in range(100):
            x = random.randint(0, self.width() or 800)
            y = random.randint(0, self.height() or 600)
            brightness = random.randint(50, 255)
            self.stars.append([x, y, brightness, random.choice([-1, 1])])
    
    def paintEvent(self, event):
        import random
        from PyQt5.QtGui import QPainter, QColor
        painter = QPainter(self)
        
        # Черный фон
        painter.fillRect(self.rect(), QColor(0, 0, 0))
        
        # Рисуем звезды
        for star in self.stars:
            x, y, brightness, direction = star
            
            # Пульсация яркости
            brightness += direction
            if brightness >= 255:
                brightness = 255
                direction = -1
            elif brightness <= 50:
                brightness = 50
                direction = 1
                
            star[2] = brightness
            star[3] = direction
            
            # Рисуем звезду (белая точка)
            painter.setPen(QColor(brightness, brightness, brightness))
            painter.drawPoint(x, y)
            
    def resizeEvent(self, event):
        # Перераспределяем звезды при изменении размера
        width, height = self.width(), self.height()
        for star in self.stars:
            if star[0] > width:
                star[0] = random.randint(0, width)
            if star[1] > height:
                star[1] = random.randint(0, height)

class DropArea(QLabel):
    fileDropped = pyqtSignal(str)
    
    def __init__(self):
        super().__init__()
        self.setAlignment(Qt.AlignCenter)
        self.setText("Перетащите видео сюда\nили нажмите +")
        self.setStyleSheet("color: white; font-size: 18px; border: 2px dashed white; border-radius: 10px; padding: 50px;")
        self.setAcceptDrops(True)
        
    def dragEnterEvent(self, event):
        if event.mimeData().hasUrls():
            event.acceptProposedAction()
            self.setStyleSheet("color: white; font-size: 18px; border: 2px dashed #3498db; border-radius: 10px; padding: 50px; background-color: rgba(52, 152, 219, 0.2);")
            
    def dragLeaveEvent(self, event):
        self.setStyleSheet("color: white; font-size: 18px; border: 2px dashed white; border-radius: 10px; padding: 50px;")
        
    def dropEvent(self, event):
        file_path = event.mimeData().urls()[0].toLocalFile()
        self.setStyleSheet("color: white; font-size: 18px; border: 2px dashed white; border-radius: 10px; padding: 50px;")
        self.fileDropped.emit(file_path)

class VideoFrame(QLabel):
    """Виджет для отображения отдельного кадра из видео"""
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumSize(160, 90)
        self.setAlignment(Qt.AlignCenter)
        self.setStyleSheet("background-color: #222; color: white; border: 1px solid #555;")
        self.setText("Нет кадра")
    
    def setFrame(self, pixmap):
        if pixmap:
            self.setPixmap(pixmap.scaled(self.width(), self.height(), Qt.KeepAspectRatio))
        else:
            self.setText("Нет кадра")

class StyledButton(QPushButton):
    """Кнопка с улучшенным стилем"""
    def __init__(self, text="", icon=None, parent=None):
        super().__init__(text, parent)
        if icon:
            self.setIcon(icon)
        
        # Устанавливаем базовый стиль кнопки
        self.setStyleSheet("""
            QPushButton {
                background-color: #3d5afe;
                color: white;
                border-radius: 5px;
                padding: 6px 12px;
                font-weight: bold;
                border: none;
            }
            QPushButton:hover {
                background-color: #536dfe;
            }
            QPushButton:pressed {
                background-color: #304ffe;
            }
            QPushButton:disabled {
                background-color: #9e9e9e;
            }
        """)
        
        # Добавляем эффект тени
        shadow = QGraphicsDropShadowEffect(self)
        shadow.setBlurRadius(10)
        shadow.setColor(QColor(0, 0, 0, 80))
        shadow.setOffset(2, 2)
        self.setGraphicsEffect(shadow)

class CircleButton(QPushButton):
    """Круглая кнопка с улучшенным стилем"""
    def __init__(self, text="", icon=None, parent=None):
        super().__init__(text, parent)
        if icon:
            self.setIcon(icon)
            self.setIconSize(QSize(16, 16))
        
        # Устанавливаем базовый стиль кнопки
        self.setStyleSheet("""
            QPushButton {
                background-color: #3d5afe;
                color: white;
                border-radius: 20px;
                padding: 8px;
                font-weight: bold;
                border: none;
                min-width: 40px;
                min-height: 40px;
                max-width: 40px;
                max-height: 40px;
            }
            QPushButton:hover {
                background-color: #536dfe;
            }
            QPushButton:pressed {
                background-color: #304ffe;
            }
            QPushButton:disabled {
                background-color: #9e9e9e;
            }
        """)
        
        # Добавляем эффект тени
        shadow = QGraphicsDropShadowEffect(self)
        shadow.setBlurRadius(10)
        shadow.setColor(QColor(0, 0, 0, 80))
        shadow.setOffset(2, 2)
        self.setGraphicsEffect(shadow)

class CustomVideoSlider(QSlider):
    """Слайдер с улучшенным дизайном и перетаскиванием как в CapCut"""
    def __init__(self, orientation, parent=None):
        super().__init__(orientation, parent)
        self.setStyleSheet("""
            QSlider::groove:horizontal {
                border: 1px solid #999999;
                height: 8px;
                background: #3d3d3d;
                margin: 2px 0;
                border-radius: 4px;
            }
            QSlider::handle:horizontal {
                background: #3d5afe;
                border: 1px solid #3d5afe;
                width: 16px;
                height: 16px;
                margin: -5px 0;
                border-radius: 8px;
            }
            QSlider::handle:horizontal:hover {
                background: #536dfe;
                border: 1px solid #536dfe;
            }
            QSlider::add-page:horizontal {
                background: #3d3d3d;
                border-radius: 4px;
            }
            QSlider::sub-page:horizontal {
                background: #536dfe;
                border-radius: 4px;
            }
        """)
        
        # Переменные для перетаскивания
        self.is_dragging = False
        
        # Эффект тени
        shadow = QGraphicsDropShadowEffect(self)
        shadow.setBlurRadius(10)
        shadow.setColor(QColor(0, 0, 0, 50))
        shadow.setOffset(0, 2)
        self.setGraphicsEffect(shadow)
    
    def mousePressEvent(self, event):
        """Отслеживает нажатие мыши для мгновенного перемещения индикатора"""
        if event.button() == Qt.LeftButton:
            self.is_dragging = True
            value = QStyle.sliderValueFromPosition(
                self.minimum(), self.maximum(), event.x(), self.width())
            self.setValue(value)
            self.sliderMoved.emit(value)
            event.accept()
        super().mousePressEvent(event)
    
    def mouseMoveEvent(self, event):
        """Обрабатывает перемещение мыши для перетаскивания индикатора"""
        if self.is_dragging:
            value = QStyle.sliderValueFromPosition(
                self.minimum(), self.maximum(), event.x(), self.width())
            self.setValue(value)
            self.sliderMoved.emit(value)
            event.accept()
        super().mouseMoveEvent(event)
    
    def mouseReleaseEvent(self, event):
        """Обрабатывает отпускание кнопки мыши"""
        if event.button() == Qt.LeftButton and self.is_dragging:
            self.is_dragging = False
            value = QStyle.sliderValueFromPosition(
                self.minimum(), self.maximum(), event.x(), self.width())
            self.setValue(value)
            self.sliderReleased.emit()
            event.accept()
        super().mouseReleaseEvent(event)
    
    def enterEvent(self, event):
        """Изменяет курсор при наведении на слайдер"""
        self.setCursor(Qt.PointingHandCursor)
        super().enterEvent(event)
    
    def leaveEvent(self, event):
        """Возвращает стандартный курсор при уходе мыши"""
        self.setCursor(Qt.ArrowCursor)
        super().leaveEvent(event)

class VLCVideoWidget(QFrame):
    """Виджет для отображения видео с помощью VLC"""
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumHeight(300)
        self.setStyleSheet("background-color: #121212; border-radius: 8px;")
        
        # Создаем экземпляр VLC и Media Player
        self.instance = vlc.Instance()
        self.media_player = self.instance.media_player_new()
        
        # Флаг для отслеживания состояния воспроизведения
        self.is_playing = False
        
        # Получаем идентификатор окна для воспроизведения
        if sys.platform.startswith('linux'):  # для Linux
            self.media_player.set_xwindow(self.winId())
        elif sys.platform == "win32":  # для Windows
            self.media_player.set_hwnd(self.winId())
        elif sys.platform == "darwin":  # для macOS
            self.media_player.set_nsobject(int(self.winId()))
    
    def play(self):
        """Воспроизводит видео"""
        self.media_player.play()
        self.is_playing = True
    
    def pause(self):
        """Приостанавливает видео"""
        self.media_player.pause()
        self.is_playing = False
    
    def set_media(self, file_path):
        """Устанавливает медиафайл для воспроизведения"""
        media = self.instance.media_new(file_path)
        self.media_player.set_media(media)
    
    def set_position(self, position):
        """Устанавливает позицию воспроизведения (0.0 - 1.0)"""
        self.media_player.set_position(position)
    
    def get_position(self):
        """Возвращает текущую позицию воспроизведения (0.0 - 1.0)"""
        return self.media_player.get_position()
    
    def get_length(self):
        """Возвращает длительность видео в миллисекундах"""
        return self.media_player.get_length()
    
    def get_time(self):
        """Возвращает текущее время воспроизведения в миллисекундах"""
        return self.media_player.get_time()
    
    def set_time(self, time_ms):
        """Устанавливает текущее время воспроизведения в миллисекундах"""
        self.media_player.set_time(time_ms)

class VideoPlayerWidget(QWidget):
    timeSelected = pyqtSignal(float, str)  # Время и тип (start/end)
    
    def __init__(self, parent=None):
        super().__init__(parent)
        
        # Основной layout
        layout = QVBoxLayout(self)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(10)
        
        # Видео плеер на базе VLC
        self.video_widget = VLCVideoWidget()
        layout.addWidget(self.video_widget)
        
        # Контроллеры видео
        control_layout = QHBoxLayout()
        control_layout.setContentsMargins(0, 5, 0, 5)
        control_layout.setSpacing(10)
        
        # Кнопка Play/Pause
        self.play_button = CircleButton()
        self.play_button.setIcon(QIcon(QApplication.style().standardIcon(QStyle.SP_MediaPlay)))
        self.play_button.clicked.connect(self.toggle_play)
        control_layout.addWidget(self.play_button)
        
        # Слайдер прогресса видео
        self.progress_slider = CustomVideoSlider(Qt.Horizontal)
        self.progress_slider.setRange(0, 1000)  # Используем 0-1000 для более точного позиционирования
        self.progress_slider.sliderMoved.connect(self.set_position)
        self.progress_slider.sliderPressed.connect(self.on_slider_pressed)
        self.progress_slider.sliderReleased.connect(self.on_slider_released)
        control_layout.addWidget(self.progress_slider)
        
        # Метка времени
        self.time_label = QLabel("00:00:00 / 00:00:00")
        self.time_label.setStyleSheet("color: white; font-weight: bold;")
        control_layout.addWidget(self.time_label)
        
        layout.addLayout(control_layout)
        
        # Контроллеры выбора начала и конца обрезки
        trim_layout = QHBoxLayout()
        trim_layout.setContentsMargins(0, 5, 0, 5)
        trim_layout.setSpacing(10)
        
        # Предпросмотр начального кадра
        self.start_frame = VideoFrame()
        
        # Кнопка установки начальной точки
        self.set_start_button = StyledButton("Установить начало")
        self.set_start_button.clicked.connect(lambda: self.set_trim_point("start"))
        
        # Предпросмотр конечного кадра
        self.end_frame = VideoFrame()
        
        # Кнопка установки конечной точки
        self.set_end_button = StyledButton("Установить конец")
        self.set_end_button.clicked.connect(lambda: self.set_trim_point("end"))
        
        # Значения времени
        self.start_time = 0
        self.end_time = 0
        self.duration = 0
        
        # Добавляем виджеты в layout
        trim_layout.addWidget(self.start_frame)
        trim_layout.addWidget(self.set_start_button)
        trim_layout.addWidget(self.set_end_button)
        trim_layout.addWidget(self.end_frame)
        
        layout.addLayout(trim_layout)
        
        # Метки времени обрезки
        times_layout = QHBoxLayout()
        times_layout.setContentsMargins(0, 5, 0, 5)
        times_layout.setSpacing(10)
        
        self.start_time_label = QLabel("Начало: 00:00:00")
        self.start_time_label.setStyleSheet("color: white; font-weight: bold;")
        
        self.end_time_label = QLabel("Конец: 00:00:00")
        self.end_time_label.setStyleSheet("color: white; font-weight: bold;")
        
        self.duration_label = QLabel("Длительность: 00:00:00")
        self.duration_label.setStyleSheet("color: white; font-weight: bold;")
        
        times_layout.addWidget(self.start_time_label)
        times_layout.addStretch()
        times_layout.addWidget(self.duration_label)
        times_layout.addStretch()
        times_layout.addWidget(self.end_time_label)
        
        layout.addLayout(times_layout)
        
        # Флаг для отслеживания состояния слайдера
        self.slider_being_dragged = False
        
        # Видеопроцессор для извлечения кадров
        self.video_processor = None
        
        # Таймер для обновления позиции слайдера
        self.update_timer = QTimer(self)
        self.update_timer.setInterval(100)  # 100 мс
        self.update_timer.timeout.connect(self.update_interface)
        self.update_timer.start()
        
        # Стилизация всего виджета
        self.setStyleSheet("""
            QWidget {
                background-color: #212121;
                color: white;
            }
        """)
        
    def set_video_processor(self, processor):
        """Устанавливает видеопроцессор для извлечения кадров"""
        self.video_processor = processor
    
    def load_video(self, file_path):
        """Загружает видео в плеер"""
        self.video_widget.set_media(file_path)
        self.play_button.setIcon(QIcon(QApplication.style().standardIcon(QStyle.SP_MediaPlay)))
        
        # Даем VLC время для загрузки видео
        QTimer.singleShot(500, self.update_duration)
    
    def update_duration(self):
        """Обновляет информацию о длительности видео"""
        self.duration = self.video_widget.get_length()
        if self.duration > 0:
            # Устанавливаем конечное время по умолчанию
            self.end_time = self.duration
            self.start_time = 0
            
            # Обновляем метки
            self.update_time_labels()
            
            # Извлекаем кадры для превью
            self.update_frame_previews()
    
    def toggle_play(self):
        """Переключает воспроизведение/паузу"""
        if self.video_widget.is_playing:
            self.video_widget.pause()
            self.play_button.setIcon(QIcon(QApplication.style().standardIcon(QStyle.SP_MediaPlay)))
        else:
            self.video_widget.play()
            self.play_button.setIcon(QIcon(QApplication.style().standardIcon(QStyle.SP_MediaPause)))
    
    def update_interface(self):
        """Обновляет интерфейс в соответствии с текущим состоянием плеера"""
        if not self.slider_being_dragged and self.video_widget.get_length() > 0:
            # Обновляем положение слайдера
            position = self.video_widget.get_position()
            if position >= 0:
                self.progress_slider.setValue(int(position * 1000))
            
            # Обновляем метку времени
            current_time = self.video_widget.get_time()
            total_time = self.video_widget.get_length()
            
            if current_time >= 0 and total_time > 0:
                current = str(datetime.timedelta(milliseconds=current_time)).split('.')[0]
                total = str(datetime.timedelta(milliseconds=total_time)).split('.')[0]
                self.time_label.setText(f"{current} / {total}")
    
    def set_position(self, value):
        """Устанавливает позицию воспроизведения при перемещении слайдера"""
        position = value / 1000.0  # Преобразуем из диапазона 0-1000 в 0.0-1.0
        self.video_widget.set_position(position)
    
    def on_slider_pressed(self):
        """Вызывается при нажатии на слайдер"""
        self.slider_being_dragged = True
        if self.video_widget.is_playing:
            self.video_widget.pause()
    
    def on_slider_released(self):
        """Вызывается при отпускании слайдера"""
        self.slider_being_dragged = False
        position = self.progress_slider.value() / 1000.0
        self.video_widget.set_position(position)
        if self.video_widget.is_playing:
            self.video_widget.play()
    
    def set_trim_point(self, point_type):
        """Устанавливает точку обрезки (начало или конец)"""
        current_time = self.video_widget.get_time()
        total_time = self.video_widget.get_length()
        
        if current_time < 0 or total_time <= 0:
            return
        
        if point_type == "start":
            if current_time < self.end_time:
                self.start_time = current_time
                self.timeSelected.emit(current_time / 1000, "start")
            else:
                # Если начальное время больше конечного, корректируем
                self.start_time = self.end_time - 1000  # 1 секунда до конца
                self.video_widget.set_time(self.start_time)
                self.timeSelected.emit(self.start_time / 1000, "start")
        else:  # end
            if current_time > self.start_time:
                self.end_time = current_time
                self.timeSelected.emit(current_time / 1000, "end")
            else:
                # Если конечное время меньше начального, корректируем
                self.end_time = self.start_time + 1000  # 1 секунда от начала
                self.video_widget.set_time(self.end_time)
                self.timeSelected.emit(self.end_time / 1000, "end")
        
        # Обновляем метки времени
        self.update_time_labels()
        
        # Обновляем превью кадров
        self.update_frame_previews()
    
    def update_time_labels(self):
        """Обновляет метки времени обрезки"""
        start = str(datetime.timedelta(milliseconds=self.start_time)).split('.')[0]
        end = str(datetime.timedelta(milliseconds=self.end_time)).split('.')[0]
        duration = str(datetime.timedelta(milliseconds=self.end_time - self.start_time)).split('.')[0]
        
        self.start_time_label.setText(f"Начало: {start}")
        self.end_time_label.setText(f"Конец: {end}")
        self.duration_label.setText(f"Длительность: {duration}")
    
    def update_frame_previews(self):
        """Обновляет превью кадров начала и конца"""
        if self.video_processor and self.video_processor.current_video:
            # Извлекаем и устанавливаем начальный кадр
            start_frame_path = self.video_processor.extract_frame(self.start_time / 1000)
            if start_frame_path:
                self.start_frame.setFrame(QPixmap(start_frame_path))
                os.remove(start_frame_path)  # Удаляем временный файл
            
            # Извлекаем и устанавливаем конечный кадр
            end_frame_path = self.video_processor.extract_frame(self.end_time / 1000)
            if end_frame_path:
                self.end_frame.setFrame(QPixmap(end_frame_path))
                os.remove(end_frame_path)  # Удаляем временный файл
    
    def get_trim_times(self):
        """Возвращает время начала и конца в секундах"""
        return self.start_time / 1000, self.end_time / 1000

class CuttingProgressDialog(QDialog):
    """Диалог прогресса обрезки видео"""
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Обработка видео")
        self.setFixedSize(400, 100)
        
        layout = QVBoxLayout(self)
        
        # Метка статуса
        self.status_label = QLabel("Подготовка к обрезке видео...")
        layout.addWidget(self.status_label)
        
        # Прогресс-бар
        self.progress_bar = QProgressBar()
        self.progress_bar.setRange(0, 100)
        self.progress_bar.setValue(0)
        layout.addWidget(self.progress_bar)
        
        # Кнопка отмены (пока не реализована)
        self.cancel_button = QPushButton("Отмена")
        self.cancel_button.setEnabled(False)  # Пока не реализовано
        layout.addWidget(self.cancel_button)
    
    def update_progress(self, value):
        """Обновляет значение прогресс-бара"""
        self.progress_bar.setValue(value)
        self.status_label.setText(f"Обрезка видео: {value}%")
        
        # Если процесс завершен, закрываем диалог
        if value >= 100:
            self.accept()

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Видео Обрезатор")
        self.setMinimumSize(800, 600)
        
        # Флаг для отслеживания состояния анимации
        self.is_animating = False
        
        # Сохраняем исходный размер и позицию для анимации
        self.normal_geometry = None
        
        # Создаем процессор видео
        self.video_processor = VideoProcessor()
        
        # Центральный виджет с фоном звездного неба
        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)
        
        # Создаем фон звездного неба
        self.bg = StarBackground(self.central_widget)
        
        # Основная вертикальная разметка
        main_layout = QVBoxLayout(self.central_widget)
        
        # Область перетаскивания
        self.drop_area = DropArea()
        self.drop_area.fileDropped.connect(self.load_video)
        main_layout.addWidget(self.drop_area)
        
        # Кнопка добавления видео
        add_button = CircleButton("+")
        add_button.setFixedSize(50, 50)
        add_button.setFont(QFont("Arial", 20))
        add_button.clicked.connect(self.open_file_dialog)
        
        # Размещаем кнопку в правом нижнем углу
        button_layout = QHBoxLayout()
        button_layout.addStretch()
        button_layout.addWidget(add_button)
        main_layout.addLayout(button_layout)
        
        # Видеоплеер (изначально скрыт)
        self.video_player = VideoPlayerWidget()
        self.video_player.setVisible(False)
        self.video_player.set_video_processor(self.video_processor)
        main_layout.addWidget(self.video_player)
        
        # Контейнер управления видео (изначально скрыт)
        self.video_controls = QWidget()
        self.video_controls.setVisible(False)
        self.video_controls.setStyleSheet("""
            QWidget {
                background-color: #292929;
                border-radius: 8px;
                padding: 10px;
            }
            QLabel {
                color: white;
                font-weight: bold;
            }
        """)
        
        video_layout = QVBoxLayout(self.video_controls)
        
        # Выбор формата вывода
        format_layout = QHBoxLayout()
        format_layout.addWidget(QLabel("Формат:"))
        
        self.format_combo = QComboBox()
        self.format_combo.addItems(["MP4", "AVI", "MOV", "MKV", "WebM", "OGG", "FLV", "WMV"])
        self.format_combo.setStyleSheet("""
            QComboBox {
                background-color: #3d5afe;
                color: white;
                padding: 6px;
                border-radius: 5px;
                min-width: 100px;
            }
            QComboBox::drop-down {
                border: none;
            }
            QComboBox::down-arrow {
                image: url(down_arrow.png);
                width: 12px;
                height: 12px;
            }
            QComboBox QAbstractItemView {
                background-color: #424242;
                color: white;
                selection-background-color: #3d5afe;
            }
        """)
        format_layout.addWidget(self.format_combo)
        
        # Кнопка обрезки
        self.cut_button = StyledButton("Обрезать")
        self.cut_button.clicked.connect(self.cut_video)
        format_layout.addWidget(self.cut_button)
        
        video_layout.addLayout(format_layout)
        
        # Добавляем контейнер управления видео в основную разметку
        main_layout.addWidget(self.video_controls)
        
        # Устанавливаем фон на задний план и растягиваем его на все окно
        self.bg.setParent(self.central_widget)
        self.bg.lower()
        
        # Подключаем сигнал прогресса к методу
        self.video_processor.progress_updated.connect(self.update_progress_dialog)
        
        # Диалог прогресса (создается при необходимости)
        self.progress_dialog = None
        
        # Создаем анимацию загрузки
        self.create_loading_animation()
        
        # Добавляем стили для всего окна
        self.setStyleSheet("""
            QMainWindow {
                background-color: #121212;
            }
        """)
        
    def resizeEvent(self, event):
        # Растягиваем фон на все окно
        self.bg.setGeometry(0, 0, self.width(), self.height())
        super().resizeEvent(event)
    
    def changeEvent(self, event):
        """Обрабатывает изменения состояния окна"""
        if event.type() == QEvent.WindowStateChange:
            # Если окно было свернуто и сейчас разворачивается
            if event.oldState() & Qt.WindowMinimized:
                # Окно развернулось после минимизации
                self.showAnimation()
            # Если окно только что было свернуто
            elif self.windowState() & Qt.WindowMinimized:
                # Окно минимизировано - запускаем анимацию сворачивания
                self.minimizeAnimation()
        super().changeEvent(event)
        
    def showAnimation(self):
        """Анимация разворачивания окна"""
        if self.is_animating:
            return
            
        self.is_animating = True
        
        # Восстанавливаем нормальное состояние
        self.showNormal()
        
        # Сохраняем целевую геометрию
        target_geometry = self.geometry()
        
        # Создаем анимацию из маленького квадрата к полному размеру
        # Начинаем с маленькой точки в центре
        center_pos = target_geometry.center()
        start_size = QRect(
            center_pos.x() - 10, 
            center_pos.y() - 10, 
            20, 20
        )
        
        self.setGeometry(start_size)
        
        # Создаем анимацию
        self.animation = QPropertyAnimation(self, b"geometry")
        self.animation.setDuration(400)  # 400 мс
        self.animation.setStartValue(start_size)
        self.animation.setEndValue(target_geometry)
        self.animation.setEasingCurve(QEasingCurve.OutCubic)
        self.animation.finished.connect(self.animationFinished)
        self.animation.start()
    
    def minimizeAnimation(self):
        """Анимация сворачивания окна"""
        if self.is_animating:
            return
            
        self.is_animating = True
        
        # Сохраняем текущую геометрию для будущего восстановления
        self.normal_geometry = self.geometry()
        
        # Определяем конечную точку для анимации (маленький квадрат в центре)
        center_pos = self.geometry().center()
        end_rect = QRect(
            center_pos.x() - 10, 
            center_pos.y() - 10, 
            20, 20
        )
        
        # Создаем анимацию
        self.animation = QPropertyAnimation(self, b"geometry")
        self.animation.setDuration(350)  # 350 мс
        self.animation.setStartValue(self.normal_geometry)
        self.animation.setEndValue(end_rect)
        self.animation.setEasingCurve(QEasingCurve.InCubic)
        self.animation.finished.connect(self.finishMinimize)
        self.animation.start()
    
    def finishMinimize(self):
        """Завершает анимацию сворачивания, реально сворачивая окно"""
        self.is_animating = False
        self.setWindowState(Qt.WindowMinimized)
        
        # Восстанавливаем нормальный размер окна (скрыто)
        if self.normal_geometry:
            self.setGeometry(self.normal_geometry)
    
    def animationFinished(self):
        """Вызывается по завершении анимации"""
        self.is_animating = False
        
    def open_file_dialog(self):
        """Открывает диалог выбора файла"""
        file_path, _ = QFileDialog.getOpenFileName(
            self, "Выберите видео", "", "Видео файлы (*.mp4 *.avi *.mov *.mkv *.webm *.flv *.wmv)"
        )
        if file_path:
            self.load_video(file_path)
    
    def load_video(self, file_path):
        """Загружает видео и отображает интерфейс для работы с ним"""
        # Показываем анимацию загрузки
        self.show_loading_animation()
        
        # Загружаем видео в отдельном потоке
        self.load_thread = LoadVideoThread(file_path, self.video_processor)
        self.load_thread.finished.connect(self.on_video_loaded)
        self.load_thread.start()
    
    def on_video_loaded(self, success):
        """Вызывается при завершении загрузки видео"""
        # Скрываем анимацию загрузки
        self.hide_loading_animation()
        
        if success:
            # Обновляем интерфейс
            self.drop_area.setVisible(False)
            self.video_player.setVisible(True)
            self.video_controls.setVisible(True)
            
            # Загружаем видео в плеер
            self.video_player.load_video(self.video_processor.current_video)
            
            # Устанавливаем обработчик выбора времени
            self.video_player.timeSelected.connect(self.update_time_range)
        else:
            print("Ошибка при загрузке видео")
    
    def update_time_range(self, time_value, time_type):
        """Обновляет диапазон временных точек обрезки"""
        if time_type == "start":
            self.start_time = time_value
        else:
            self.end_time = time_value
    
    def cut_video(self):
        """Обрезает видео согласно выбранным параметрам"""
        # Получаем время начала и конца
        start_time, end_time = self.video_player.get_trim_times()
        
        # Запрашиваем путь для сохранения
        output_format = self.format_combo.currentText()
        default_extension = f".{output_format.lower()}"
        
        # Создаем диалог сохранения файла
        output_path, _ = QFileDialog.getSaveFileName(
            self, "Сохранить обрезанное видео", "", 
            f"{output_format} Files (*{default_extension});;All Files (*)"
        )
        
        if output_path:
            # Создаем и показываем диалог прогресса
            self.progress_dialog = CuttingProgressDialog(self)
            self.progress_dialog.show()
            
            # Запускаем обрезку в отдельном потоке
            self.cut_thread = CutVideoThread(
                self.video_processor, start_time, end_time, 
                output_path, output_format
            )
            self.cut_thread.finished.connect(self.on_cutting_finished)
            self.cut_thread.start()
    
    def update_progress_dialog(self, progress):
        """Обновляет диалог прогресса обрезки"""
        if self.progress_dialog:
            self.progress_dialog.update_progress(progress)
    
    def on_cutting_finished(self, success):
        """Вызывается по завершении процесса обрезки"""
        if self.progress_dialog:
            self.progress_dialog.hide()
            self.progress_dialog = None
        
        if success:
            print("Видео успешно обрезано")
        else:
            print("Ошибка при обрезке видео")

    def create_loading_animation(self):
        """Создает анимацию загрузки"""
        self.loading_label = QLabel(self)
        self.loading_label.setAlignment(Qt.AlignCenter)
        self.loading_label.setStyleSheet("background-color: rgba(0, 0, 0, 150); border-radius: 10px;")
        
        # Создаем анимацию
        self.loading_movie = QMovie("loading.gif")
        if not os.path.exists("loading.gif"):
            # Если анимации нет, создаем новую
            create_loading_animation()
            self.loading_movie = QMovie("loading.gif")
        
        self.loading_movie.setScaledSize(QSize(100, 100))
        self.loading_label.setMovie(self.loading_movie)
        self.loading_label.setFixedSize(150, 150)
        self.loading_label.hide()
    
    def show_loading_animation(self):
        """Показывает анимацию загрузки"""
        # Позиционируем анимацию по центру окна
        center_x = (self.width() - self.loading_label.width()) // 2
        center_y = (self.height() - self.loading_label.height()) // 2
        self.loading_label.move(center_x, center_y)
        
        # Запускаем и показываем анимацию
        self.loading_movie.start()
        self.loading_label.show()
        self.loading_label.raise_()
    
    def hide_loading_animation(self):
        """Скрывает анимацию загрузки"""
        self.loading_movie.stop()
        self.loading_label.hide()
    
    def resizeEvent(self, event):
        # Растягиваем фон на все окно
        self.bg.resize(self.size())
        
        # При изменении размера окна, центрируем анимацию загрузки
        if hasattr(self, 'loading_label') and self.loading_label.isVisible():
            center_x = (self.width() - self.loading_label.width()) // 2
            center_y = (self.height() - self.loading_label.height()) // 2
            self.loading_label.move(center_x, center_y)
        
        super().resizeEvent(event)

class LoadVideoThread(QThread):
    finished = pyqtSignal(bool)
    
    def __init__(self, file_path, processor):
        super().__init__()
        self.file_path = file_path
        self.processor = processor
    
    def run(self):
        """Загружает видео в отдельном потоке"""
        success = self.processor.load_video(self.file_path)
        self.finished.emit(success)

class CutVideoThread(QThread):
    finished = pyqtSignal(bool)
    
    def __init__(self, processor, start_time, end_time, output_path, output_format):
        super().__init__()
        self.processor = processor
        self.start_time = start_time
        self.end_time = end_time
        self.output_path = output_path
        self.output_format = output_format
    
    def run(self):
        """Обрезает видео в отдельном потоке"""
        success = self.processor.cut_video(
            self.start_time, self.end_time, 
            self.output_path, self.output_format
        )
        self.finished.emit(success)

# Создаем GIF с анимацией загрузки (для использования в заставке)
def create_loading_gif():
    if not os.path.exists('loading.gif'):
        # Создаем анимацию через наш скрипт
        create_loading_animation()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    
    # Создаем GIF с анимацией загрузки, если еще не создан
    create_loading_gif()
    
    # Показываем экран заставки
    splash = SplashScreen()
    splash.show()
    
    sys.exit(app.exec_()) 