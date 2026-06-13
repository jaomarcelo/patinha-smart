from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

import threading
import serial
import time
import os

import models
from database import SessionLocal, engine


# cria tabelas automaticamente
models.Base.metadata.create_all(bind=engine)

app = FastAPI()


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


def ler_cabo_arduino():
    try:

        porta_usb = serial.Serial(
            "COM3",
            9600,
            timeout=1
        )

        time.sleep(2)

        print("Arduino conectado!")

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

                    try:

                        db = SessionLocal()

                        nova_leitura = models.LeituraPote(
                            nivel_racao=nivel
                        )

                        db.add(nova_leitura)
                        db.commit()

                        db.close()

                    except Exception as erro:

                        print(
                            f"Erro banco: {erro}"
                        )

    except Exception as erro:
        print(
            f"Arduino não conectado: {erro}"
        )


# só executa Arduino localmente
if os.getenv("RENDER") is None:

    thread = threading.Thread(
        target=ler_cabo_arduino,
        daemon=True
    )

    thread.start()


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

    try:

        db = SessionLocal()

        nova_leitura = models.LeituraPote(
            nivel_racao=novo_valor
        )

        db.add(nova_leitura)
        db.commit()

        db.close()

    except Exception as erro:

        print(
            f"Erro banco: {erro}"
        )

    return {
        "mensagem": f"Nível alterado para {novo_valor}%"
    }