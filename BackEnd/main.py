from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
import serial
import threading
import time

import models
from database import SessionLocal, engine

# Cria as tabelas
# models.Base.metadata.create_all(bind=engine)

app = FastAPI(title="Patinha Smart API")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

estado_atual_do_pote = {
    "racao": 0,
    "wifi": True
}


def salvar_leitura(nivel):
    db = SessionLocal()

    try:
        leitura = models.LeituraPote(
            nivel_racao=nivel
        )

        db.add(leitura)
        db.commit()

    except Exception as erro:
        db.rollback()
        print(f"Erro ao salvar no banco: {erro}")

    finally:
        db.close()


def ler_cabo_arduino():
    try:
        porta_usb = serial.Serial(
            "COM3",
            9600,
            timeout=1
        )

        time.sleep(2)

        print("Arduino conectado")

        while True:

            if porta_usb.in_waiting > 0:

                linha = (
                    porta_usb
                    .readline()
                    .decode("utf-8")
                    .strip()
                )

                if linha.isdigit():

                    nivel = int(linha)

                    estado_atual_do_pote["racao"] = nivel

                    salvar_leitura(nivel)

    except Exception as erro:
        print(f"Erro USB: {erro}")


# Só inicia Arduino localmente
try:
    thread = threading.Thread(
        target=ler_cabo_arduino,
        daemon=True
    )

    thread.start()

except:
    print("Arduino não encontrado")


@app.get("/")
def home():
    return {
        "mensagem": "Patinha Smart API Online"
    }


@app.get("/status")
def get_status():
    return estado_atual_do_pote


@app.get("/mudar/{novo_valor}")
def mudar_valor(novo_valor: int):

    estado_atual_do_pote["racao"] = novo_valor

    salvar_leitura(novo_valor)

    return {
        "mensagem":
        f"Nível alterado para {novo_valor}%"
    }