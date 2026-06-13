from sqlalchemy import Column, Integer, DateTime
from sqlalchemy.sql import func

from database import Base

class LeituraPote(Base):
    __tablename__ = "leituras"

    id = Column(Integer, primary_key=True, index=True)

    nivel_racao = Column(Integer)

    data_hora = Column(
        DateTime(timezone=True),
        server_default=func.now()
    )