from sqlalchemy import (
    Column, Integer, String, Timestamp
)
from sqlalchemy.sql import func
from .base import Base

class Account(Base):
    __tablename__ = "accounts"

    id = Column(Integer, primary_key=True)
    email = Column(String(100), unique=True, nullable=False)
    password = Column(String(255))
    role = Column(String(20), nullable=False, default="user")
    created_at = Column(Timestamp, server_default=func.now())
    updated_at = Column(Timestamp, onupdate=func.now())
