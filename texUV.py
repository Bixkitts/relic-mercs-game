# This script opens a GUI window for picking
# points on a texture and getting UV coordinates
import argparse
import matplotlib.pyplot as plt
import matplotlib.text as mtext

def main(image_path):
    image = plt.imread(image_path)
    height, width, _ = image.shape

    fig, ax = plt.subplots()
    ax.imshow(image)

    # Initialize a Text object for displaying the coordinates
    coord_text = ax.text(0, 0, '', color='red', fontsize=12, backgroundcolor='white', bbox=dict(facecolor='white', edgecolor='red', boxstyle='round,pad=0.5'))
    coord_text.set_visible(False)  # Initially invisible

    def onclick(event):
        x, y = event.xdata, event.ydata
        if x is not None and y is not None:
            u = x / width
            v = 1 - y / height
            # Update text and position
            coord_text.set_position((x, y))
            coord_text.set_text(f'UV: ({u:.4f}, {v:.4f})')
            coord_text.set_visible(True)
            fig.canvas.draw()  # Redraw canvas

    fig.canvas.mpl_connect('button_press_event', onclick)
    plt.show()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Pick points on an image and get UV coordinates.')
    parser.add_argument('image_path', type=str, help='Path to the image file.')
    args = parser.parse_args()

    main(args.image_path)

