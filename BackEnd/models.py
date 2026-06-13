from sqlalchemy import Column
from sqlalchemy import Integer
from sqlalchemy import DateTime

from datetime import datetime

from database import Base


class LeituraPote(Base):
    __tablename__ = "leituras"

    id = Column(
        Integer,
        primary_key=True,
        index=True
    )

    nivel_racao = Column(
        Integer,
        nullable=False
    )

    data_hora = Column(
        DateTime,
        default=datetime.utcnow
    )