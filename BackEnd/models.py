from sqlalchemy import Column, Integer, DateTime
import datetime
from database import Base

class LeituraPote(Base):
    __tablename__ = "leituras"

    id = Column(Integer, primary_key=True, index=True)
    nivel_racao = Column(Integer)
    
    data_hora = Column(DateTime, default=datetime.datetime.now)