from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
import serial
import threading
import time


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

def ler_cabo_arduino():
    try:
        
        porta_usb = serial.Serial('COM3', 9600, timeout=1)
        time.sleep(2) 
        
        print("Lendo dados do Arduino via USB...")
        
        while True:
            if porta_usb.in_waiting > 0:
                linha = porta_usb.readline().decode('utf-8').strip()
                
                if linha.isdigit():
                    nivel = int(linha)
                    estado_atual_do_pote["racao"] = nivel
                    print(f"Chegou do Arduino: {nivel}%")
                    
                    
                    try:
                        db = SessionLocal() 
                        nova_leitura = models.LeituraPote(nivel_racao=nivel)
                        db.add(nova_leitura)
                        db.commit() 
                        db.close()  
                        print(" -> Salvo no MySQL com sucesso!")
                    except Exception as erro_db:
                        print(f" -> Erro ao salvar no banco: {erro_db}")
                    
    except Exception as e:
        print(f"Erro na porta USB: {e}")
        print("Verifique se colocou a porta COM certa e se o Monitor Serial da IDE está FECHADO.")


thread = threading.Thread(target=ler_cabo_arduino, daemon=True)
thread.start()


@app.get("/status")
def get_status():
    
    return estado_atual_do_pote

# ---  (TESTE SEM ARDUINO) ---
@app.get("/mudar/{novo_valor}")
def mudar_valor_manualmente(novo_valor: int):
    
    estado_atual_do_pote["racao"] = novo_valor
    
    try:
        db = SessionLocal()
        nova_leitura = models.LeituraPote(nivel_racao=novo_valor)
        db.add(nova_leitura)
        db.commit()
        db.close()
    except Exception as e:
        print(f"Erro ao simular no banco: {e}")
    
    return {"mensagem": f"Sucesso! O nível do pote foi forçado para {novo_valor}%"}