from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

import models
from database import SessionLocal, engine

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


@app.get("/")
def home():
    return {
        "mensagem": "Patinha Smart Online"
    }


@app.get("/status")
def status():
    return estado_atual_do_pote


@app.get("/mudar/{novo_valor}")
def mudar_valor(novo_valor: int):

    estado_atual_do_pote["racao"] = novo_valor

    db = SessionLocal()

    nova_leitura = models.LeituraPote(
        nivel_racao=novo_valor
    )

    db.add(nova_leitura)
    db.commit()
    db.close()

    return {
        "mensagem": "Valor atualizado"
    }


@app.get("/historico")
def historico():

    db = SessionLocal()

    leituras = db.query(
        models.LeituraPote
    ).all()

    resultado = []

    for leitura in leituras:

        resultado.append({
            "id": leitura.id,
            "racao": leitura.nivel_racao,
            "data": str(leitura.data_hora)
        })

    db.close()

    return resultado