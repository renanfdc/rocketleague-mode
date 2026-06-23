import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from PIL import Image, ImageTk, ImageDraw
import json, os, shutil, glob

class ModManager:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("RL Ball Mod Manager")
        self.root.geometry("750x650")
        self.root.configure(bg="#1a1a2e")
        self.root.resizable(False, False)

        # Detectar pasta do jogo
        self.game_dir = self.find_game_dir()
        self.mod_dir = os.path.join(self.game_dir, "BallMod") if self.game_dir else None
        self.config = {}
        self.load_config()

        self.build_ui()
        self.root.mainloop()

    def find_game_dir(self):
        # Sanitizado para repo publico: o original tinha uma lista de caminhos
        # absolutos de instalacao (Steam/Epic em varias unidades). Aqui detectamos
        # pela variavel de ambiente RL_GAME_DIR ou varrendo as unidades em busca da
        # subpasta padrao, sem expor o layout de nenhuma maquina.
        env = os.environ.get("RL_GAME_DIR")
        if env and os.path.exists(os.path.join(env, "RocketLeague.exe")):
            return env
        rel = os.path.join("steamapps", "common", "rocketleague", "Binaries", "Win64")
        for letter in "CDEFG":
            for base in (
                os.path.join(letter + ":\\", "Steam"),
                os.path.join(letter + ":\\", "SteamLibrary"),
                os.path.join(letter + ":\\", "Program Files (x86)", "Steam"),
                os.path.join(letter + ":\\", "Epic Games", "rocketleague", "Binaries", "Win64"),
            ):
                cand = base if base.endswith("Win64") else os.path.join(base, rel)
                if os.path.exists(os.path.join(cand, "RocketLeague.exe")):
                    return cand
        return None

    def load_config(self):
        if self.mod_dir and os.path.exists(os.path.join(self.mod_dir, "config.json")):
            with open(os.path.join(self.mod_dir, "config.json")) as f:
                self.config = json.load(f)
        else:
            self.config = {
                "enabled": True, "texture_file": "gg_lag_team.png",
                "auto_test": False, "night_mode": True,
                "night_intensity": 0.3, "log_level": "info"
            }

    def save_config(self):
        if self.mod_dir:
            with open(os.path.join(self.mod_dir, "config.json"), "w") as f:
                json.dump(self.config, f, indent=4)
            self.show_toast("Configuracao salva! A bola muda em ~5 segundos.")

    def build_ui(self):
        # Titulo
        title = tk.Label(self.root, text="⚽ RL BALL MOD", font=("Segoe UI", 24, "bold"),
                        fg="#00f0ff", bg="#1a1a2e")
        title.pack(pady=10)

        # Status
        status_text = f"✅ Jogo encontrado" if self.game_dir else "❌ Jogo nao encontrado"
        status = tk.Label(self.root, text=status_text, font=("Segoe UI", 9),
                         fg="#00ff88" if self.game_dir else "#ff4444", bg="#1a1a2e")
        status.pack()

        # Frame principal
        main = tk.Frame(self.root, bg="#1a1a2e")
        main.pack(fill="both", expand=True, padx=20, pady=10)

        # Coluna esquerda - Preview + Texturas
        left = tk.Frame(main, bg="#222244", relief="ridge", bd=1)
        left.pack(side="left", fill="both", expand=True, padx=(0,10))

        tk.Label(left, text="🎱 Textura da Bola", font=("Segoe UI", 12, "bold"),
                fg="#00f0ff", bg="#222244").pack(pady=(10,5))

        # Preview circular
        self.preview_frame = tk.Frame(left, bg="#222244", width=160, height=160)
        self.preview_frame.pack(pady=5)
        self.preview_label = tk.Label(self.preview_frame, bg="#111")
        self.preview_label.pack()
        self.update_preview()

        # Botoes de textura
        tex_frame = tk.Frame(left, bg="#222244")
        tex_frame.pack(fill="x", padx=10, pady=5)

        textures = [
            ("🟡 GG Lag Team", "gg_lag_team.png"),
            ("🐞 Joaninha", "joaninha_ball.png"),
            ("🟢 Alta Visibilidade", "high_contrast_ball.png"),
        ]

        for text, file in textures:
            btn = tk.Button(tex_frame, text=text, font=("Segoe UI", 10),
                          bg="#2a2a4a", fg="#e0e0e0", activebackground="#3a3a5a",
                          relief="flat", cursor="hand2", pady=6,
                          command=lambda f=file: self.select_texture(f))
            btn.pack(fill="x", pady=2)

        # Botao enviar textura
        tk.Button(left, text="📁 Enviar nova textura PNG", font=("Segoe UI", 10),
                 bg="#1a2a3e", fg="#00f0ff", relief="flat", cursor="hand2", pady=8,
                 command=self.upload_texture).pack(fill="x", padx=10, pady=(5,10))

        # Coluna direita - Configuracoes
        right = tk.Frame(main, bg="#222244", relief="ridge", bd=1)
        right.pack(side="right", fill="both", expand=True)

        tk.Label(right, text="⚙️ Configuracoes", font=("Segoe UI", 12, "bold"),
                fg="#00f0ff", bg="#222244").pack(pady=(10,5))

        # Toggles
        self.var_enabled = tk.BooleanVar(value=self.config.get("enabled", True))
        self.var_night = tk.BooleanVar(value=self.config.get("night_mode", True))

        toggle_frame = tk.Frame(right, bg="#222244")
        toggle_frame.pack(fill="x", padx=15, pady=5)

        tk.Checkbutton(toggle_frame, text="  Mod Ativo", variable=self.var_enabled,
                       font=("Segoe UI", 11), bg="#222244", fg="#e0e0e0",
                       selectcolor="#333", activebackground="#222244",
                       command=self.on_config_change).pack(anchor="w", pady=4)

        tk.Checkbutton(toggle_frame, text="  🌙 Ceu Noturno", variable=self.var_night,
                       font=("Segoe UI", 11), bg="#222244", fg="#e0e0e0",
                       selectcolor="#333", activebackground="#222244",
                       command=self.on_config_change).pack(anchor="w", pady=4)

        # Slider intensidade
        slider_frame = tk.Frame(right, bg="#222244")
        slider_frame.pack(fill="x", padx=15, pady=10)

        tk.Label(slider_frame, text="Escuridao do Ceu:", font=("Segoe UI", 10),
                fg="#ccc", bg="#222244").pack(anchor="w")

        self.night_val = tk.Label(slider_frame, text=f"{self.config.get('night_intensity', 0.3):.2f}",
                                 font=("Segoe UI", 12, "bold"), fg="#ff00ff", bg="#222244")
        self.night_val.pack(anchor="e")

        self.slider = tk.Scale(slider_frame, from_=10, to=90, orient="horizontal",
                              bg="#222244", fg="#e0e0e0", troughcolor="#333",
                              highlightthickness=0, length=250, showvalue=False,
                              command=self.on_slider_change)
        self.slider.set(int(self.config.get("night_intensity", 0.3) * 100))
        self.slider.pack(fill="x")

        tk.Label(slider_frame, text="← mais escuro          mais claro →",
                font=("Segoe UI", 8), fg="#666", bg="#222244").pack()

        # Textura atual
        self.tex_label = tk.Label(right, text=f"Textura: {self.config.get('texture_file', '?')}",
                                 font=("Segoe UI", 10), fg="#aaa", bg="#222244")
        self.tex_label.pack(pady=10)

        # Botoes de acao
        btn_frame = tk.Frame(right, bg="#222244")
        btn_frame.pack(fill="x", padx=15, pady=10)

        tk.Button(btn_frame, text="🚀 APLICAR", font=("Segoe UI", 12, "bold"),
                 bg="#8b00ff", fg="white", relief="flat", cursor="hand2", pady=8,
                 command=self.apply_config).pack(fill="x", pady=3)

        tk.Button(btn_frame, text="🔄 Recarregar", font=("Segoe UI", 10),
                 bg="#2a2a4a", fg="#00f0ff", relief="flat", cursor="hand2", pady=5,
                 command=self.reload_config).pack(fill="x", pady=3)

        # Toast
        self.toast = tk.Label(self.root, text="", font=("Segoe UI", 10, "bold"),
                             fg="#000", bg="#00ff88")

    def update_preview(self):
        try:
            tex_file = self.config.get("texture_file", "")
            tex_path = os.path.join(self.mod_dir, "textures", tex_file) if self.mod_dir else ""
            if os.path.exists(tex_path):
                img = Image.open(tex_path).convert("RGB")
                img = img.resize((150, 150), Image.LANCZOS)
                # Mascara circular
                mask = Image.new("L", (150, 150), 0)
                ImageDraw.Draw(mask).ellipse([3,3,147,147], fill=255)
                bg = Image.new("RGB", (150, 150), (17, 17, 17))
                bg.paste(img, (0,0), mask)
                self.preview_img = ImageTk.PhotoImage(bg)
                self.preview_label.config(image=self.preview_img, width=150, height=150)
            else:
                self.preview_label.config(text="Sem preview", fg="#666", width=20, height=8)
        except:
            self.preview_label.config(text="Erro", fg="#ff4444", width=20, height=8)

    def select_texture(self, filename):
        self.config["texture_file"] = filename
        self.tex_label.config(text=f"Textura: {filename}")
        self.update_preview()
        self.save_config()

    def upload_texture(self):
        path = filedialog.askopenfilename(
            title="Escolha uma textura PNG",
            filetypes=[("Imagens", "*.png *.jpg *.jpeg")]
        )
        if path and self.mod_dir:
            name = os.path.basename(path)
            dest = os.path.join(self.mod_dir, "textures", name)
            shutil.copy2(path, dest)
            self.config["texture_file"] = name
            self.update_preview()
            self.save_config()
            self.show_toast(f"Textura '{name}' instalada!")

    def on_config_change(self):
        self.config["enabled"] = self.var_enabled.get()
        self.config["night_mode"] = self.var_night.get()

    def on_slider_change(self, val):
        self.config["night_intensity"] = int(val) / 100
        self.night_val.config(text=f"{self.config['night_intensity']:.2f}")

    def apply_config(self):
        self.on_config_change()
        self.save_config()

    def reload_config(self):
        self.load_config()
        self.var_enabled.set(self.config.get("enabled", True))
        self.var_night.set(self.config.get("night_mode", True))
        self.slider.set(int(self.config.get("night_intensity", 0.3) * 100))
        self.tex_label.config(text=f"Textura: {self.config.get('texture_file', '?')}")
        self.update_preview()
        self.show_toast("Config recarregado!")

    def show_toast(self, msg):
        self.toast.config(text=f"  {msg}  ")
        self.toast.place(relx=0.5, rely=0.95, anchor="center")
        self.root.after(3000, lambda: self.toast.place_forget())

if __name__ == "__main__":
    ModManager()
